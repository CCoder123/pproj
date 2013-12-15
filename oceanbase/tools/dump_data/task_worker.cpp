#include <new>
#include <stdio.h>
#include "common/ob_define.h"
#include "common/ob_result.h"
#include "common/ob_malloc.h"
#include "common/utility.h"
#include "task_packet.h"
#include "task_worker.h"

using namespace oceanbase::common;
using namespace oceanbase::tools;

TaskWorker::TaskWorker(const ObServer & server, const int64_t timeout, const uint64_t retry_times)
{
  server_ = server;
  timeout_ = timeout;
  retry_times_ = retry_times;
  temp_result_ = NULL;
  output_path_ = "/tmp/";
}

TaskWorker::~TaskWorker()
{
  if (NULL != temp_result_)
  {
    delete []temp_result_;
    temp_result_ = NULL;
  }
}

int TaskWorker::init(RpcStub * rpc, const char * data_path)
{
  int ret = OB_SUCCESS;
  if ((NULL == rpc) || (temp_result_ != NULL))
  {
    TBSYS_LOG(ERROR, "check data path or rpc stub failed:path[%s], temp_result[%p], stub[%p]",
        data_path, temp_result_, rpc);
    ret = OB_INPUT_PARAM_ERROR;
  }
  else
  {
    rpc_ = rpc;
    temp_result_ = new(std::nothrow) char[OB_MAX_PACKET_LENGTH];
    if (NULL == temp_result_)
    {
      TBSYS_LOG(ERROR, "check new memory failed:mem[%p]", temp_result_);
      ret = OB_ERROR;
    }
    output_path_ = data_path;
  }
  return ret;
}

int TaskWorker::fetch_task(TaskCounter & count, TaskInfo & task)
{
  int ret = OB_SUCCESS;
  if (check_inner_stat())
  {
    for (uint64_t i = 0; i <= MAX_RETRY_TIMES; ++i)
    {
      ret = rpc_->fetch_task(server_, timeout_, count, task);
      if (ret != OB_SUCCESS)
      {
        TBSYS_LOG(WARN, "check fetch task failed:ret[%d]", ret);
        usleep(RETRY_INTERVAL * (i + 1));
      }
      else
      {
        TBSYS_LOG(INFO, "fetch task succ:task[%lu]", task.get_id());
        // the param is from local rpc buffer so deserialize to its own buffer
        ret = task.set_param(task.get_param());
        if (ret != OB_SUCCESS)
        {
          TBSYS_LOG(ERROR, "set task its own param failed:ret[%d]", ret);
        }
        break;
      }
    }
  }
  else
  {
    ret = OB_ERROR;
    TBSYS_LOG(ERROR, "%s", "check inner stat failed");
  }
  return ret;
}


int TaskWorker::report_task(const int64_t result_code, const char * file_name, const TaskInfo & task)
{
  int ret = OB_SUCCESS;
  if ((NULL != file_name) && check_inner_stat())
  {
    for (uint64_t i = 0; i <= MAX_RETRY_TIMES; ++i)
    {
      ret = rpc_->report_task(server_, timeout_, result_code, file_name, task);
      if (ret != OB_SUCCESS)
      {
        TBSYS_LOG(WARN, "check report task failed:ret[%d]", ret);
        usleep(RETRY_INTERVAL);
      }
      else
      {
        break;
      }
    }
  }
  else
  {
    ret = OB_ERROR;
    TBSYS_LOG(ERROR, "check inner stat or file_name failed:file[%s]", file_name);
  }
  return ret;
}

int TaskWorker::scan(const int64_t index, const TabletLocation & list, const ObScanParam & param,
    ObScanner & result)
{
  int ret = OB_SUCCESS;
  if (check_inner_stat())
  {
    ret = rpc_->scan(index, list, timeout_, param, result);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(WARN, "check scan task failed:ret[%d]", ret);
    }
  }
  else
  {
    ret = OB_ERROR;
    TBSYS_LOG(ERROR, "%s", "check inner stat failed");
  }
  return ret;
}

int TaskWorker::do_task(FILE * file, const TaskInfo & task)
{
  int ret = OB_SUCCESS;
  ObScanner scanner;
  ObScanParam param = task.get_param();
  while(true)
  {
    // every modify need a new buffer
    // no reset temp buffer
    ObMemBuffer temp_buffer;
    ret = scan(task.get_index(), task.get_location(), param, scanner);
    if (ret != OB_SUCCESS)
    {
      param.get_range()->hex_dump();
      TBSYS_LOG(ERROR, "scan failed:task[%lu], ret[%d]", task.get_id(), ret);
      break;
    }
    else
    {
      ret = output(file, scanner);
      if (ret != OB_SUCCESS)
      {
        TBSYS_LOG(ERROR, "output result failed:task[%lu], ret[%d]", task.get_id(), ret);
        break;
      }
      bool finish = false;
      ret = check_finish(param, scanner, finish);
      if (ret != OB_SUCCESS)
      {
        TBSYS_LOG(ERROR, "check result finish failed:task[%lu], ret[%d]", task.get_id(), ret);
        break;
      }
      else if (!finish)
      {
        ret = modify_param(scanner, temp_buffer, param);
        if (ret != OB_SUCCESS)
        {
          TBSYS_LOG(ERROR, "modify param for next query failed:task[%lu], ret[%d]",
              task.get_id(), ret);
          break;
        }
      }
      else
      {
        TBSYS_LOG(INFO, "do the task succ:task[%lu]", task.get_id());
        break;
      }
    }
  }
  return ret;
}

int TaskWorker::do_task(const char * file_name, const TaskInfo & task)
{
  int ret = OB_SUCCESS;
  char temp_name[MAX_PATH_LEN] = "";
  snprintf(temp_name, sizeof(temp_name), "%s.temp", file_name);
  FILE * file = fopen(temp_name, "w");
  if (NULL == file)
  {
    ret = OB_ERROR;
    TBSYS_LOG(ERROR, "check fopen failed:file[%s]", temp_name);
  }
  else
  {
    ret = do_task(file, task);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "check do task failed:task[%lu], ret[%d]", task.get_id(), ret);
    }
  }

  int err = OB_SUCCESS;
  if (NULL != file)
  {
    err = fclose(file);
    if (OB_SUCCESS != err)
    {
      TBSYS_LOG(ERROR, "fclose failed:file[%s], err[%d], do_task ret[%d]", temp_name, err, ret);
      err = OB_ERROR;
    }
    else if (OB_SUCCESS == ret)
    {
      err = rename(temp_name, file_name);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(ERROR, "rename failed from temp to dest:temp[%s], dest[%s], err[%d]",
            temp_name, file_name, err);
        err = OB_ERROR;
      }
    }
    
    // remove failed temp file
    if (ret != OB_SUCCESS) 
    {
      err = remove(temp_name);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(ERROR, "remove temp file failed:temp[%s], err[%d]", temp_name, err);
        err = OB_ERROR;
      }
    }
  }
  return (err | ret);
}

int TaskWorker::do_task(const TaskInfo & task, char file_name[], const int64_t len)
{
  int ret = OB_SUCCESS;
  ObScanParam param = task.get_param();
  snprintf(file_name, len, "%s/%ld_%.*s_%ld_%ld", output_path_, (int64_t)getpid(),
      param.get_table_name().length(), param.get_table_name().ptr(), 
      task.get_token(), task.get_id());
  if (NULL != output_path_)
  {
    ret = do_task(file_name, task);
  }
  else
  {
    ret = do_task(stdout, task);
  }
  return ret;
}


int TaskWorker::check_finish(const ObScanParam & param, const ObScanner & result, bool & finish)
{
  int64_t item_count = 0;
  int ret = result.get_is_req_fullfilled(finish, item_count);
  if (ret != OB_SUCCESS)
  {
    TBSYS_LOG(ERROR, "get is fullfilled failed:ret[%d]", ret);
  }
  else
  {
    ret = check_result(param, result);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "check result failed:ret[%d]", ret);
    }
  }
  return ret;
}

int TaskWorker::modify_param(const ObScanner & result, ObMemBuffer & buffer, ObScanParam & param)
{
  ObString last_rowkey;
  int ret = result.get_last_row_key(last_rowkey);
  if (ret != OB_SUCCESS)
  {
    TBSYS_LOG(ERROR, "get last row key failed:ret[%d]", ret);
  }
  else
  {
    const ObRange * range = NULL;
    range = param.get_range();
    if (NULL == range)
    {
      TBSYS_LOG(ERROR, "get range failed:range[%p]", range);
    }
    else
    {
      const_cast<ObRange *>(range)->border_flag_.unset_min_value();
      const_cast<ObRange *>(range)->border_flag_.unset_inclusive_start();
      if (NULL == buffer.malloc(last_rowkey.length()))
      {
        TBSYS_LOG(ERROR, "check malloc mem buffer failed");
        ret = OB_ERROR;
      }
      else
      {
        memcpy(buffer.get_buffer(), last_rowkey.ptr(), last_rowkey.length());
        const_cast<ObRange *>(range)->start_key_.assign(
            reinterpret_cast<char*>(buffer.get_buffer()), last_rowkey.length());
        param.set(param.get_table_id(), param.get_table_name(), *range);
      }
    }
  }
  return ret;
}

int TaskWorker::check_result(const ObScanParam & param, const ObScanner & result)
{
  int ret = OB_SUCCESS;
  return ret;
}

int TaskWorker::output(FILE * file, ObScanner & result) const
{
  int ret = OB_SUCCESS;
  if (NULL == file)
  {
    TBSYS_LOG(ERROR, "check output file failed:file[%p]", file);
    ret = OB_ERROR;
  }
  else if (!check_inner_stat())
  {
    TBSYS_LOG(ERROR, "%s", "check inner stat failed");
    ret = OB_ERROR;
  }
  else
  {
    int64_t pos = 0;
    ret = result.serialize(temp_result_, OB_MAX_PACKET_LENGTH, pos);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "check temp result serialize failed:ret[%d]", ret);
    }
    else
    {
      int64_t size = fwrite(temp_result_, 1, pos, file);
      if (size != pos)
      {
        TBSYS_LOG(ERROR, "fwrite file failed:size[%ld], pos[%ld]", size, pos);
        size = result.get_serialize_size();
        if (size != pos)
        {
          TBSYS_LOG(ERROR, "check scanner size failed:size[%ld], pos[%ld]", size, pos);
        }
        ret = OB_ERROR;
      }
    }
  }
  return ret;
}


