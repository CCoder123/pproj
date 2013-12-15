
#include "client_rpc.h"
#include "common/ob_result.h"
#include "common/utility.h"
#include "base_client.h"

using namespace oceanbase::common;
using namespace oceanbase::rootserver;

ObClientRpcStub::ObClientRpcStub()
  : frame_buffer_(FRAME_BUFFER_SIZE)
{
  init_ = false;
  rpc_frame_ = NULL;
}

ObClientRpcStub::~ObClientRpcStub()
{

}


int ObClientRpcStub::initialize(const ObServer & remote_server, 
    const ObClientManager * rpc_frame)
{
  int ret = OB_SUCCESS;
  if (init_ || (NULL == rpc_frame))
  {
    TBSYS_LOG(ERROR, "already inited or check input failed:inited[%s], rpc_frame[%p]",
       (init_? "ture": "false"), rpc_frame);
    ret = OB_ERROR; 
  }
  else
  {
    remote_server_ = remote_server;
    rpc_frame_ = rpc_frame;
    init_ = true;
  }
  return ret;
}


int ObClientRpcStub::get_frame_buffer(ObDataBuffer & data_buffer) const
{
  int ret = OB_SUCCESS;
  if (!check_inner_stat())
  {
    TBSYS_LOG(ERROR, "check inner stat failed.");
    ret = OB_ERROR;
  }

  ThreadSpecificBuffer::Buffer* rpc_buffer = frame_buffer_.get_buffer();
  if (OB_SUCCESS == ret)
  {
    if (NULL == rpc_buffer)
    {
      TBSYS_LOG(ERROR, "get thread rpc buff failed:buffer[%p].", rpc_buffer);
      ret = OB_ERROR;
    }
    else
    {
      rpc_buffer->reset();
      data_buffer.set_data(rpc_buffer->current(), rpc_buffer->remain());
    }
  }
  return ret;
}


int ObClientRpcStub::cs_scan(const ObScanParam& scan_param, ObScanner& scanner)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  // step 1. serialize timestamp to data_buff
  if (OB_SUCCESS == ret)
  {
    ret = scan_param.serialize(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position());
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize scan_param failed:ret[%d].", ret);
    }
  }

  // step 2. send request for fetch new schema
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_SCAN_REQUEST, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for scan failed"
          " ret[%d].",  ret);
    }
  }


  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }
  // step 4. deserialize the table schema
  if (OB_SUCCESS == ret)
  {
    ret = scanner.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize scanner from buff failed"
          "pos[%ld], ret[%d].", pos, ret);
    }
  }

  return ret;
}


int ObClientRpcStub::cs_get(const ObGetParam& get_param, ObScanner& scanner)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  // step 1. serialize timestamp to data_buff
  if (OB_SUCCESS == ret)
  {
    ret = get_param.serialize(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position());
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize get_param failed:ret[%d].", ret);
    }
  }

  // step 2. send request for fetch new schema
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_GET_REQUEST, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for get failed"
          " ret[%d].",  ret);
    }
  }


  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }
  // step 4. deserialize the table schema
  if (OB_SUCCESS == ret)
  {
    ret = scanner.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize scanner from buff failed"
          "pos[%ld], ret[%d].", pos, ret);
    }
  }

  return ret;
}

int ObClientRpcStub::cs_dump_tablet_image(
    const int32_t index, const int32_t disk_no,  
    oceanbase::common::ObString &image_buf)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  // step 1. serialize timestamp to data_buff
  if (OB_SUCCESS == ret)
  {
    ret = serialization::encode_vi32(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position(), index);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize index failed:ret[%d].", ret);
    }
  }
  if (OB_SUCCESS == ret)
  {
    ret = serialization::encode_vi32(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position(), disk_no);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize disk_no failed:ret[%d].", ret);
    }
  }
  // step 2. send request for fetch new schema
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_CS_DUMP_TABLET_IMAGE, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for cs_dump_tablet_image failed"
          " ret[%d].",  ret);
    }
  }


  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }
  // step 4. deserialize the table schema
  if (OB_SUCCESS == ret)
  {
    ret = image_buf.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize image_buf from buff failed"
          "pos[%ld], ret[%d].", pos, ret);
    }
  }

  return ret;
}

int ObClientRpcStub::rs_dump_cs_info(ObChunkServerManager &obcsm)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  // step 1. serialize timestamp to data_buff

  // step 2. send request for fetch new schema
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_DUMP_CS_INFO, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for cs_dump_tablet_image failed"
          " ret[%d].",  ret);
    }
  }


  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }
  // step 4. deserialize the table schema
  if (OB_SUCCESS == ret)
  {
    ret = obcsm.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize ObChunkServerManager from buff failed"
          "pos[%ld], ret[%d].", pos, ret);
    }
  }

  return ret;
}


int ObClientRpcStub::fetch_stats(oceanbase::common::ObStatManager &obsm)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  // step 1. serialize timestamp to data_buff
  //
  int64_t start_time = tbsys::CTimeUtil::getTime();

  // step 2. send request for fetch new schema
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_FETCH_STATS, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for fetch_stats failed"
          " ret[%d].",  ret);
    }
  }

  int64_t end_time = tbsys::CTimeUtil::getTime();
  if (OB_SUCCESS != ret)
  {
      TBSYS_LOG(ERROR, "send request to remote server for fetch_stats failed"
          " ret[%d].consume=%ld",  ret, end_time-start_time);
  }

  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }
  // step 4. deserialize the table schema
  if (OB_SUCCESS == ret)
  {
    ret = obsm.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize ObChunkServerManager from buff failed"
          "pos[%ld], ret[%d].", pos, ret);
    }
  }

  return ret;
}

int ObClientRpcStub::get_update_server(ObServer &update_server)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);
  // step 1. send get update server info request
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, OB_GET_UPDATE_SERVER_INFO, DEFAULT_VERSION,
        timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to root server for register server failed:ret[%d]", ret);
    }
  }

  // step 2. deserialize restult code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d]", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }

  // step 3. deserialize update server addr
  if (OB_SUCCESS == ret)
  {
    ret = update_server.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "deserialize server failed:pos[%ld], ret[%d]", pos, ret);
    }
  }
  return ret;
}

int ObClientRpcStub::start_merge(const int64_t frozen_memtable_version, const int32_t init_flag)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  // step 1. serialize frozen_memtable_version to data_buff
  if (OB_SUCCESS == ret)
  {
    ret = serialization::encode_vi64(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position(), frozen_memtable_version);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize frozen_memtable_version failed:ret[%d].", ret);
    }
  }
  if (OB_SUCCESS == ret)
  {
    ret = serialization::encode_vi32(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position(), init_flag);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize init_flag failed:ret[%d].", ret);
    }
  }

  // step 2. send request for fetch new schema
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_START_MERGE, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for start_merge failed"
          " ret[%d].",  ret);
    }
  }


  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }

  return ret;
}

int ObClientRpcStub::drop_tablets(const int64_t frozen_memtable_version)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  // step 1. serialize frozen_memtable_version to data_buff
  if (OB_SUCCESS == ret)
  {
    ret = serialization::encode_vi64(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position(), frozen_memtable_version);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize frozen_memtable_version failed:ret[%d].", ret);
    }
  }

  // step 2. send request for fetch new schema
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_START_MERGE, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for start_merge failed"
          " ret[%d].",  ret);
    }
  }


  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }

  return ret;
}

int ObClientRpcStub::start_gc(const int32_t reserve)
{
  int ret = OB_SUCCESS;
  const int64_t timeout = 1000000;  // send_request timeout millionseconds
  ObDataBuffer data_buff;
  ret = get_frame_buffer(data_buff);

  if (OB_SUCCESS == ret)
  {
    ret = serialization::encode_vi32(data_buff.get_data(), 
        data_buff.get_capacity(), data_buff.get_position(), reserve);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR,"encode failed ret[%d]",ret);
    }
  }

  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(remote_server_, 
        OB_CS_START_GC, DEFAULT_VERSION, timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR, "send request to remote server for cs_dump_tablet_image failed"
          " ret[%d].",  ret);
    }
  }

  // step 3. deserialize the response code
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result_code failed:pos[%ld], ret[%d].", pos, ret);
    }
    else
    {
      ret = result_code.result_code_;
    }
  }
  // step 4. deserialize the table schema
  if (OB_SUCCESS == ret)
  {
    TBSYS_LOG(INFO,"send start gc success");
  }
  
  return ret;
}

int ObClientRpcStub::rs_scan(const ObServer & server, const int64_t timeout, 
    const ObScanParam & param, ObScanner & result)
{
  ObDataBuffer data_buff;
  int ret = get_frame_buffer(data_buff);
  if (ret != OB_SUCCESS)
  {
    TBSYS_LOG(ERROR, "check get rpc buffer failed:ret[%d]", ret);
  }
  else
  {
    // step 1. serialize ObGetParam to the data_buff
    ret = param.serialize(data_buff.get_data(), data_buff.get_capacity(), 
        data_buff.get_position());
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "serialize get param failed:ret[%d]", ret);
    }
  }

  // step 2. send request for get data 
  if (OB_SUCCESS == ret)
  {
    ret = rpc_frame_->send_request(server, OB_SCAN_REQUEST, DEFAULT_VERSION, 
        timeout, data_buff);
    if (ret != OB_SUCCESS)
    {
      TBSYS_LOG(WARN, "get data failed from server:ret[%d]", ret);
    }
  }
  
  // step 3. deserialize the response result
  int64_t pos = 0;
  if (OB_SUCCESS == ret)
  {
    ObResultCode result_code;
    ret = result_code.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize result failed:pos[%ld], ret[%d]", pos, ret);
    }
    else if ((ret = result_code.result_code_) != OB_SUCCESS)
    {
      TBSYS_LOG(ERROR,"scan failed,ret[%d]",ret);
    }
  }
  
  // step 4. deserialize the scanner 
  if (OB_SUCCESS == ret)
  {
    result.clear();
    ret = result.deserialize(data_buff.get_data(), data_buff.get_position(), pos);
    if (OB_SUCCESS != ret)
    {
      TBSYS_LOG(ERROR, "deserialize scanner from buff failed:pos[%ld], ret[%d]", pos, ret);
    }
  }
  return ret;
}

int ObClientRpcStub::get_tablet_info(const uint64_t table_id, const char* table_name,
    const ObRange& range, ObTabletLocation location [],int32_t& size)
{
  int ret = OB_SUCCESS;
  int32_t index = 0;
  const int64_t timeout = 1000000;
  if (OB_INVALID_ID == table_id || size <= 0)
  {
    TBSYS_LOG(ERROR,"invalid table id");
    ret = OB_ERROR;
  }
  
  ObScanParam param;
  ObScanner scanner;
  ObString table_name_str;

  table_name_str.assign_ptr(const_cast<char*>(table_name), strlen(table_name));
  
  
  if (OB_SUCCESS == ret)
  {
    param.set(OB_INVALID_ID,table_name_str,range); //use table name
  }

  if ((OB_SUCCESS == ret) && ((ret = rs_scan(remote_server_,timeout,param,scanner)) != OB_SUCCESS) )
  {
    TBSYS_LOG(ERROR,"get tablet from rootserver failed:[%d]",ret);
  }
  
  ObServer server;
  char tmp_buf[32];
  ObString start_key;
  ObString end_key; 
  ObCellInfo * cell = NULL;
  ObScannerIterator iter; 
  bool row_change = false;
   
  if (OB_SUCCESS == ret) 
  { 
    int64_t ip = 0;
    int64_t port = 0;
    int64_t version = 0;
    iter = scanner.begin();
    for (; OB_SUCCESS == ret && iter != scanner.end() && index < size; ++iter)
    {
      ret = iter.get_cell(&cell, &row_change);

      fprintf(stderr, "row_change=%d, column_name=%.*s\n", row_change, 
          cell->column_name_.length(), cell->column_name_.ptr());
      hex_dump(cell->row_key_.ptr(), cell->row_key_.length());
      cell->value_.dump();

      if (ret != OB_SUCCESS)
      {
        TBSYS_LOG(ERROR, "get cell from scanner iterator failed:ret[%d]", ret);
        break;
      }
      
      if (row_change)
      {          
        if (OB_SUCCESS == ret)
        {
          TBSYS_LOG(DEBUG,"row changed");
          if (0 == port || 0 == ip  || 0 == version)
          {
            TBSYS_LOG(DEBUG, "%s,version:%ld", "check failed",version);
          }
          else
          {
            server.set_ipv4_addr(ip, port);
            server.to_string(tmp_buf,sizeof(tmp_buf));
            TBSYS_LOG(INFO,"add tablet s:%s,%ld",tmp_buf,version);
            ObTabletLocation addr(version, server);
            location[index++] = addr;
          }
          ip = port = version = 0;
        }
      }

      if (OB_SUCCESS == ret && cell != NULL)
      {
        end_key.assign(cell->row_key_.ptr(), cell->row_key_.length());
        if ((cell->column_name_.compare("1_port") == 0) 
            || (cell->column_name_.compare("2_port") == 0) 
            || (cell->column_name_.compare("3_port") == 0))
        {
          ret = cell->value_.get_int(port);
          TBSYS_LOG(DEBUG,"port is %ld",port);
        }
        else if ((cell->column_name_.compare("1_ipv4") == 0)
            || (cell->column_name_.compare("2_ipv4") == 0)
            || (cell->column_name_.compare("3_ipv4") == 0))
        {
          ret = cell->value_.get_int(ip);
          TBSYS_LOG(DEBUG,"ip is %ld",ip);
        }
        else if (cell->column_name_.compare("1_tablet_version") == 0 ||
                 cell->column_name_.compare("2_tablet_version") == 0 ||
                 cell->column_name_.compare("3_tablet_version") == 0)
        {
          ret = cell->value_.get_int(version);
          hex_dump(cell->row_key_.ptr(),cell->row_key_.length(),false,TBSYS_LOG_LEVEL_INFO);
          TBSYS_LOG(DEBUG,"tablet_version is %d",version);
        }

        if (ret != OB_SUCCESS)
        {
          TBSYS_LOG(ERROR, "check get value failed:ret[%d]", ret);
          break;
        }
      }
    }

    if (OB_SUCCESS == ret && ip != 0 && port != 0 && version != 0 && index < size) //last row
    {
      server.set_ipv4_addr(ip, port);
      ObTabletLocation addr(version, server);
      server.to_string(tmp_buf,sizeof(tmp_buf));
      TBSYS_LOG(DEBUG,"add tablet s:%s,%ld",tmp_buf,version);
      location[index++] = addr;
    }
  }

  if (OB_SUCCESS == ret)
  {
    size = index;
    TBSYS_LOG(DEBUG,"get %d tablets from rootserver",size);
  }
  return ret;
}
