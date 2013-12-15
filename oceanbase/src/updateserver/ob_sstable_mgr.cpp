/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_sstable_mgr.cpp for ...
 *
 * Authors:
 *   yubai <yubai.lk@taobao.com>
 *
 */
#include "common/ob_define.h"
#include "common/ob_atomic.h"
#include "common/file_directory_utils.h"
#include "sstable/ob_sstable_writer.h"
#include "ob_sstable_mgr.h"
#include "ob_update_server_main.h"
#include "ob_multi_file_utils.h"

namespace oceanbase
{
  namespace updateserver
  {
    using namespace common;
    using namespace hash;

    int ObUpsFetchParam::clone(const ObUpsFetchParam& rp)
    {
      int ret = OB_SUCCESS;

      min_log_id_ = rp.min_log_id_;
      max_log_id_ = rp.max_log_id_;
      ckpt_id_ = rp.ckpt_id_;
      fetch_log_ = rp.fetch_log_;
      fetch_ckpt_ = rp.fetch_ckpt_;

      fetch_sstable_ = rp.fetch_sstable_;

      SSTList::iterator ssti;
      for (ssti = rp.sst_list_.begin(); ssti != rp.sst_list_.end(); ++ssti)
      {
        ret = add_sst_file_info(*ssti);
        if (OB_SUCCESS != ret)
        {
          TBSYS_LOG(ERROR, "add_sst_file_info error, ret=%d name=%s path=%s",
              ret, ssti->name.ptr(), ssti->path.ptr());
          break;
        }
      }

      return ret;
    }

    int ObUpsFetchParam::add_sst_file_info(const SSTFileInfo &sst_file_info)
    {
      int err = OB_SUCCESS;

      SSTFileInfo sst_fi;
      err = string_buf_.write_string(sst_file_info.path, &(sst_fi.path));
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(ERROR, "ObStringBuf write_string error, err=%d");
      }
      else
      {
        err = string_buf_.write_string(sst_file_info.name, &(sst_fi.name));
        if (OB_SUCCESS != err)
        {
          TBSYS_LOG(ERROR, "ObStringBuf write_string error, err=%d");
        }
      }

      if (OB_SUCCESS == err)
      {
        err = sst_list_.push_back(sst_fi);
        if (OB_SUCCESS != err)
        {
          TBSYS_LOG(ERROR, "ObVector push_back error, err=%d sst_list_.size()=%d",
              err, sst_list_.size());
        }
      }

      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(ERROR, "err=%d sst_file_info.path=\"%.*s\"(path.length=%ld path.size=%ld)"
            " sst_file_info.name=\"%.*s\"(name.length=%ld name.size=%ld)",
            err, sst_file_info.path.length(), sst_file_info.path.ptr(),
            sst_file_info.path.length(), sst_file_info.path.size(),
            sst_file_info.name.length(), sst_file_info.name.ptr(),
            sst_file_info.name.length(), sst_file_info.name.size());
      }

      return err;
    }

    int64_t ObUpsFetchParam::get_serialize_size(void) const
    {
      int64_t size = 0;

      size += serialization::encoded_length_i64(min_log_id_);
      size += serialization::encoded_length_i64(max_log_id_);
      size += serialization::encoded_length_i64(ckpt_id_);
      size += serialization::encoded_length_bool(fetch_log_);
      size += serialization::encoded_length_bool(fetch_ckpt_);
      size += serialization::encoded_length_bool(fetch_sstable_);

      size += serialization::encoded_length_i32(sst_list_.size());

      for (SSTList::iterator iter = sst_list_.begin(); iter != sst_list_.end(); iter++)
      {
        size += iter->path.get_serialize_size();
        size += iter->name.get_serialize_size();
      }

      return size;
    }

    int ObUpsFetchParam::serialize(char* buf, const int64_t buf_len, int64_t& pos) const
    {
      int err = OB_SUCCESS;

      if (OB_SUCCESS == err)
      {
        err = serialization::encode_i64(buf, buf_len, pos, min_log_id_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::encode_i64(buf, buf_len, pos, max_log_id_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::encode_i64(buf, buf_len, pos, ckpt_id_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::encode_bool(buf, buf_len, pos, fetch_log_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::encode_bool(buf, buf_len, pos, fetch_ckpt_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::encode_bool(buf, buf_len, pos, fetch_sstable_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::encode_i32(buf, buf_len, pos, sst_list_.size());
      }
      if (OB_SUCCESS == err)
      {
        for (SSTList::iterator iter = sst_list_.begin(); OB_SUCCESS == err && iter != sst_list_.end(); iter++)
        {
          if (OB_SUCCESS == err)
          {
            err = iter->path.serialize(buf, buf_len, pos);
          }
          if (OB_SUCCESS == err)
          {
            err = iter->name.serialize(buf, buf_len, pos);
          }
        }
      }

      if (err != OB_SUCCESS)
      {
        TBSYS_LOG(WARN, "ObFetchParam serialize error, err=%d buf=%p pos=%ld buf_len=%ld serialize_size=%ld",
            err, buf, pos, buf_len, get_serialize_size());
      }
      else
      {
        TBSYS_LOG(DEBUG, "min_log_id=%lu max_log_id=%lu fetch_log_=%d fetch_sstable_=%d sst_list_.size()=%d",
            min_log_id_, max_log_id_, fetch_log_, fetch_sstable_, sst_list_.size());
      }

      return err;
    }

    int ObUpsFetchParam::deserialize(const char* buf, const int64_t buf_len, int64_t& pos)
    {
      int err = OB_SUCCESS;

      int32_t sst_num = 0;
      SSTFileInfo sst_file;

      if (OB_SUCCESS == err)
      {
        err = serialization::decode_i64(buf, buf_len, pos, reinterpret_cast<int64_t*>(&min_log_id_));
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::decode_i64(buf, buf_len, pos, reinterpret_cast<int64_t*>(&max_log_id_));
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::decode_i64(buf, buf_len, pos, reinterpret_cast<int64_t*>(&ckpt_id_));
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::decode_bool(buf, buf_len, pos, &fetch_log_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::decode_bool(buf, buf_len, pos, &fetch_ckpt_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::decode_bool(buf, buf_len, pos, &fetch_sstable_);
      }
      if (OB_SUCCESS == err)
      {
        err = serialization::decode_i32(buf, buf_len, pos, &sst_num);
      }

      if (OB_SUCCESS == err)
      {
        for (int32_t i = 0; OB_SUCCESS == err && i < sst_num; i++)
        {
          if (OB_SUCCESS == err)
          {
            err = sst_file.path.deserialize(buf, buf_len, pos);
          }
          if (OB_SUCCESS == err)
          {
            err = sst_file.name.deserialize(buf, buf_len, pos);
          }
          if (OB_SUCCESS == err)
          {
            err = add_sst_file_info(sst_file);
            if (OB_SUCCESS != err)
            {
              TBSYS_LOG(WARN, "ObFetchParam add_sst_file_info error, err=%d", err);
            }
          }
        }
      }

      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "ObFetchParam deserialize error, err=%d buf=%p buf_len=%ld pos=%ld",
            err, buf, buf_len, pos);
      }
      else
      {
        TBSYS_LOG(DEBUG, "min_log_id=%lu max_log_id=%lu fetch_log_=%d fetch_sstable_=%d sst_list_.size()=%d",
            min_log_id_, max_log_id_, fetch_log_, fetch_sstable_, sst_list_.size());
      }

      return err;
    }

    StoreInfo::StoreInfo() : sstable_info_(NULL), store_handle_(StoreMgr::INVALID_HANDLE),
                             fd_(-1), ref_cnt_(0)
    {
    }

    StoreInfo::~StoreInfo()
    {
      destroy();
    }

    void StoreInfo::init(SSTableInfo *sstable_info, const StoreMgr::Handle store_handle)
    {
      sstable_info_ = sstable_info;
      store_handle_ = store_handle;
      ref_cnt_ = 1;
    }

    void StoreInfo::destroy()
    {
      if (-1 != fd_)
      {
        close(fd_);
        fd_ = -1;
      }
    }

    const char *StoreInfo::get_dir() const
    {
      const char *ret = NULL;
      SSTableMgr *sstable_mgr = NULL;
      if (NULL != sstable_info_
          && NULL != (sstable_mgr = sstable_info_->get_sstable_mgr()))
      {
        ret = sstable_mgr->get_store_mgr().get_dir(store_handle_);
      }
      return ret;
    }

    int StoreInfo::get_fd_(int &fd, int mode) const
    {
      SSTableMgr *sstable_mgr = NULL;
      if (-1 == fd
          && NULL != sstable_info_
          && NULL != (sstable_mgr = sstable_info_->get_sstable_mgr()))
      {
        const char *path = sstable_mgr->get_store_mgr().get_dir(store_handle_);
        const char *fname_substr = NULL;
        const char *fpath = NULL;
        if (NULL != path
            && NULL != (fname_substr = sstable_info_->get_fname_substr())
            && NULL != (fpath = SSTableMgr::build_str("%s/%s%s", path, fname_substr, SSTABLE_SUFFIX)))
        {
          int tmp_fd = open(fpath, mode);
          if (-1 == tmp_fd)
          {
            // TODO report broken and report report enfile
          }
          if (-1 != (int32_t)atomic_compare_exchange((uint32_t*)&fd, tmp_fd, (uint32_t)-1))
          {
            close(tmp_fd);
          }
        }
      }
      return fd;
    }

    int StoreInfo::get_fd() const
    {
      return get_fd_(fd_, FILE_OPEN_DIRECT_RFLAG);
    }

    int64_t StoreInfo::inc_ref_cnt()
    {
      return atomic_inc((uint64_t*)&ref_cnt_);
    }

    int64_t StoreInfo::dec_ref_cnt()
    {
      return atomic_dec((uint64_t*)&ref_cnt_);
    }

    bool StoreInfo::store_handle_match(const StoreMgr::Handle other) const
    {
      return (store_handle_ == other);
    }

    void StoreInfo::remove_sstable_file()
    {
      SSTableMgr *sstable_mgr = NULL;
      if (NULL != sstable_info_
          && NULL != (sstable_mgr = sstable_info_->get_sstable_mgr()))
      {
        const char *path = sstable_mgr->get_store_mgr().get_dir(store_handle_);
        const char *fname_substr = sstable_info_->get_fname_substr();
        const char *fpath = NULL;
        const char *trash_fpath = NULL;
        if (NULL != path
            && NULL != fname_substr)
        {
          fpath = SSTableMgr::build_str("%s/%s%s", path, fname_substr, SSTABLE_SUFFIX);
          trash_fpath = SSTableMgr::build_str("%s/%s/%s%s.%ld",
                        path, TRASH_DIR, fname_substr, SSTABLE_SUFFIX, tbsys::CTimeUtil::getTime());
        }
        if (NULL == fpath
            || NULL == trash_fpath)
        {
          TBSYS_LOG(WARN, "build sstable fpath fail path=[%s] fpath=[%s] trash_fpath=[%s]",
                    path, fpath, trash_fpath);
        }
        else if (0 != rename(fpath, trash_fpath))
        {
          TBSYS_LOG(WARN, "rename from [%s] to [%s] fail errno=%u", fpath, trash_fpath, errno);
        }
        else
        {
          TBSYS_LOG(INFO, "rename from [%s] to [%s] succ", fpath, trash_fpath);
        }
      }
    }

    SSTableInfo *StoreInfo::get_sstable_info() const
    {
      return sstable_info_;
    }

    StoreMgr::Handle StoreInfo::get_store_handle() const
    {
      return store_handle_;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    SSTableInfo::SSTableInfo() : ref_cnt_(0), sstable_mgr_(NULL),
                                 sstable_id_(OB_INVALID_ID), clog_id_(OB_INVALID_ID),
                                 loop_pos_(0), store_num_(0)
    {
      memset(store_infos_, 0, sizeof(store_infos_));
    }

    SSTableInfo::~SSTableInfo()
    {
      destroy();
    }

    int SSTableInfo::init(SSTableMgr *sstable_mgr, const uint64_t sstable_id, const uint64_t clog_id)
    {
      int ret = OB_SUCCESS;
      if (NULL == sstable_mgr
          || OB_INVALID_ID == sstable_id
          || OB_INVALID_ID == clog_id)
      {
        ret = OB_INVALID_ARGUMENT;
      }
      else
      {
        sstable_mgr_ = sstable_mgr;
        sstable_id_ = sstable_id;
        clog_id_ = clog_id;
        ref_cnt_ = 1;
      }
      return ret;
    }

    void SSTableInfo::destroy()
    {
      if (NULL != sstable_mgr_)
      {
        for (int64_t i = 0; i < store_num_; i++)
        {
          StoreInfo *store_info = store_infos_[i];
          if (NULL != store_info
              && 0 == store_info->dec_ref_cnt())
          {
            sstable_mgr_->get_store_allocator().deallocate(store_infos_[i]);
          }
        }
      }
      sstable_id_ = OB_INVALID_ID;
      clog_id_ = OB_INVALID_ID;
      sstable_mgr_ = NULL;
      store_num_ = 0;
      loop_pos_ = 0;
    }

    int SSTableInfo::add_store(const StoreMgr::Handle store_handle)
    {
      int ret = OB_SUCCESS;
      if (MAX_STORE_NUM <= (store_num_ + 1))
      {
        ret = OB_SIZE_OVERFLOW;
      }
      else
      {
        for (int64_t i = 0; i < store_num_; i++)
        {
          if (NULL != store_infos_[i]
              && store_infos_[i]->store_handle_match(store_handle))
          {
            ret = OB_ENTRY_EXIST;
            break;
          }
        }
        if (OB_SUCCESS == ret)
        {
          StoreInfo *store_info = NULL;
          if (NULL != sstable_mgr_
              && NULL != (store_info = sstable_mgr_->get_store_allocator().allocate()))
          {
            store_info->init(this, store_handle);
            store_infos_[store_num_++] = store_info;
          }
        }
      }
      return ret;
    }

    int SSTableInfo::erase_store(const StoreMgr::Handle store_handle)
    {
      int ret = OB_SUCCESS;
      if (0 >= store_num_)
      {
        ret = OB_ENTRY_NOT_EXIST;
      }
      else
      { 
        ret = OB_ENTRY_NOT_EXIST;
        for (int64_t i = 0; i < store_num_; i++)
        {
          StoreInfo *store_info = store_infos_[i];
          if (NULL != store_info
              && store_info->store_handle_match(store_handle))
          {
            if (0 == store_info->dec_ref_cnt())
            {
              if (NULL == sstable_mgr_)
              {
                TBSYS_LOG(WARN, "sstable_mgr null pointer cannot deallocate store_info=%p", store_info);
              }
              else
              {
                sstable_mgr_->get_store_allocator().deallocate(store_info);
              }
            }
            if ((i + 1) < store_num_)
            {
              memmove(&store_infos_[i], &store_infos_[i + 1], sizeof(StoreInfo*) * (store_num_ - i - 1));
            }
            store_num_--;
            ret = OB_SUCCESS;
            break;
          }
        }
      }
      return ret;
    };

    int64_t SSTableInfo::get_store_num() const
    {
      return store_num_;
    }

    uint64_t SSTableInfo::get_sstable_id() const
    {
      return sstable_id_;
    }

    uint64_t SSTableInfo::get_clog_id() const
    {
      return clog_id_;
    }

    void SSTableInfo::remove_sstable_file()
    {
      for (int64_t i = 0; i < store_num_; i++)
      {
        StoreInfo *store_info = store_infos_[i];
        if (NULL != store_info)
        {
          store_info->remove_sstable_file();
        }
      }
    }

    int64_t SSTableInfo::inc_ref_cnt()
    {
      return atomic_inc((uint64_t*)&ref_cnt_);
    }

    int64_t SSTableInfo::dec_ref_cnt()
    {
      return atomic_dec((uint64_t*)&ref_cnt_);
    }

    StoreInfo *SSTableInfo::get_store_info()
    {
      StoreInfo *ret = NULL;
      if (0 != store_num_)
      {
        int64_t store_pos = (int64_t)atomic_inc((uint64_t*)&loop_pos_) % store_num_;
        ret = store_infos_[store_pos];
      }
      return ret;
    }

    SSTableMgr *SSTableInfo::get_sstable_mgr() const
    {
      return sstable_mgr_;
    }

    const char *SSTableInfo::get_fname_substr() const
    {
      return SSTableMgr::sstable_id2str(sstable_id_, clog_id_);
    }

    void SSTableInfo::log_sstable_info() const
    {
      char buffer[MAX_STORE_NUM * 11] = {'\0'};
      int tmp_pos = 0;
      int tmp_ret = 0;
      int tmp_size = MAX_STORE_NUM * 11;
      for (int64_t i = 0; i < store_num_; i++)
      {
        tmp_ret = snprintf(&buffer[tmp_pos], tmp_size, "%lu,", store_infos_[i]->get_store_handle());
        if (tmp_ret >= tmp_size)
        {
          break;
        }
        else
        {
          tmp_pos += tmp_ret;
          tmp_size -= tmp_ret;
        }
      }
      SSTableID sst_id = sstable_id_;
      TBSYS_LOG(INFO, "%s store_num=%ld handle=[%s]", sst_id.log_str(), store_num_, buffer);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    SSTableMgr::SSTableMgr() : inited_(false)
    {
    }

    SSTableMgr::~SSTableMgr()
    {
      destroy();
    }

    int SSTableMgr::init(const char *store_root, const char *raid_regex, const char *dir_regex)
    {
      int ret = OB_SUCCESS;
      if (inited_)
      {
        ret = OB_INIT_TWICE;
      }
      else if (NULL == store_root
              || NULL == raid_regex
              || NULL == dir_regex)
      {
        TBSYS_LOG(WARN, "invalid param store_root=%p raid_regex=%p dir_regex=%p",
                  store_root, raid_regex, dir_regex);
        ret = OB_INVALID_ARGUMENT;
      }
      else if (0 != sstable_info_map_.create(SSTABLE_NUM))
      {
        TBSYS_LOG(WARN, "create sstable_info_map fail");
        ret = OB_ERROR;
      }
      else if (OB_SUCCESS != (ret = store_mgr_.init(store_root, raid_regex, dir_regex)))
      {
        TBSYS_LOG(WARN, "init store_mgr fail ret=%d store_root=[%s] raid_regex=[%s] dir_regex=[%s]",
                  ret, store_root, raid_regex, dir_regex);
      }
      else if (!sstable_fname_regex_.init(SSTABLE_FNAME_REGEX, REG_EXTENDED))
      {
        TBSYS_LOG(WARN, "init sstable fname regex fail regex=[%s]", SSTABLE_FNAME_REGEX);
      }
      else
      {
        TBSYS_LOG(INFO, "init sstable mgr succ store_root=[%s] raid_regex=[%s] dir_regex=[%s]",
                  store_root, raid_regex, dir_regex);
        inited_ = true;
      }
      if (OB_SUCCESS != ret)
      {
        store_mgr_.destroy();
        sstable_info_map_.destroy();
        sstable_fname_regex_.destroy();
      }
      return ret;
    }

    void SSTableMgr::destroy()
    {
      if (inited_)
      {
        SSTableInfoMap::iterator iter;
        for (iter = sstable_info_map_.begin(); iter != sstable_info_map_.end(); iter++)
        {
          sstable_allocator_.deallocate(iter->second);
        }
        store_mgr_.destroy();
        sstable_info_map_.destroy();
        sstable_fname_regex_.destroy();
        observer_list_.clear();
        inited_ = false;
      }
    }

    bool SSTableMgr::copy_sstable_file_(const uint64_t sstable_id, const uint64_t clog_id,
                                        const StoreMgr::Handle src_handle, const StoreMgr::Handle dest_handle)
    {
      bool bret = false;
      const char *src_path = store_mgr_.get_dir(src_handle);
      const char *dest_path = store_mgr_.get_dir(dest_handle);
      const char *fname_substr = sstable_id2str(sstable_id, clog_id);
      const char *src_fname = NULL;
      const char *dest_fname = NULL;
      if (NULL == src_path
          || NULL == dest_path
          || NULL == fname_substr)
      {
        TBSYS_LOG(WARN, "invalid src_path=[%s] dest_path=[%s] fname_substr=[%s]", src_path, dest_path, fname_substr);
      }
      else if (NULL == (src_fname = build_str("%s%s", fname_substr, SSTABLE_SUFFIX))
              || NULL == (dest_fname= build_str("%s%s", fname_substr, SSTABLE_SUFFIX)))
      {
        TBSYS_LOG(WARN, "build fpath fail src_fname=[%s] dest_fname=[%s] src_path=[%s] dest_path=[%s] fname_substr=[%s]",
                  src_fname, dest_fname, src_path, dest_path, fname_substr);
      }
      else if (OB_SUCCESS != FSU::cp(src_path, src_fname, dest_path, dest_fname))
      {
        TBSYS_LOG(WARN, "copy %s/%s to %s/%s fail errno=%u", src_path, src_fname, dest_path, dest_fname, errno);
      }
      else
      {
        TBSYS_LOG(INFO, "copy %s/%s to %s/%s succ", src_path, src_fname, dest_path, dest_fname);
        bret = true;
      }
      return bret;
    }

    bool SSTableMgr::build_sstable_file_(const uint64_t sstable_id, const ObString &fpaths, const int64_t time_stamp, IRowIterator &iter)
    {
      bool bret = false;
      sstable::ObSSTableSchema sstable_schema;
      ObString compressor_str;
      int store_type = 0;
      int64_t block_size = 0;
      ObUpdateServerMain *ups_main = ObUpdateServerMain::get_instance();
      if (!iter.get_sstable_schema(sstable_schema))
      {
        TBSYS_LOG(WARN, "get sstable schema fail sstable_id=%lu", sstable_id);
      }
      else if (!iter.get_compressor_name(compressor_str))
      {
        TBSYS_LOG(WARN, "get compressor name fail sstable_id=%lu", sstable_id);
      }
      else if (!iter.get_store_type(store_type))
      {
        TBSYS_LOG(WARN, "get store type fail sstable_id=%lu", sstable_id);
      }
      else if (!iter.get_block_size(block_size))
      {
        TBSYS_LOG(WARN, "get block size fail sstable_id=%lu", sstable_id);
      }
      if (NULL == ups_main)
      {
        TBSYS_LOG(WARN, "get ups main fail");
      }
      else
      {
        sstable::ObSSTableWriter sstable_writer;
        sstable::ObTrailerParam sstable_trailer_param;
        MultiFileUtils multi_file_utils;
        sstable_writer.set_file_sys(multi_file_utils);
        sstable_trailer_param.compressor_name_ = compressor_str;
        sstable_trailer_param.table_version_ = sstable_id;
        sstable_trailer_param.store_type_ = store_type;
        sstable_trailer_param.block_size_ = block_size;
        sstable_trailer_param.frozen_time_ = time_stamp;
        int tmp_ret = sstable_writer.create_sstable(sstable_schema, fpaths, sstable_trailer_param);
        if (OB_SUCCESS != tmp_ret)
        {
          TBSYS_LOG(WARN, "sstable create fail ret=%d sstable_id=%lu", tmp_ret, sstable_id);
        }
        else
        {
          sstable::ObSSTableRow sstable_row;
          int64_t approx_space_usage = 0;
          while (OB_SUCCESS == (tmp_ret = iter.next_row()))
          {
            if (OB_SUCCESS != (tmp_ret = iter.get_row(sstable_row)))
            {
              TBSYS_LOG(WARN, "get row fail ret=%d", tmp_ret);
              break;
            }
            else if (OB_SUCCESS != (tmp_ret = sstable_writer.append_row(sstable_row, approx_space_usage)))
            {
              TBSYS_LOG(WARN, "append row fail ret=%d table_id=%lu", tmp_ret, sstable_row.get_table_id());
              break;
            }
            else
            {
              TBSYS_LOG(DEBUG, "append row succ ret=%d table_id=%lu approx_space_usage=%ld",
                        tmp_ret, sstable_row.get_table_id(), approx_space_usage);
            }
            if (ObRoleMgr::ACTIVE != ups_main->get_update_server().get_role_mgr().get_state()
                && ObRoleMgr::INIT != ups_main->get_update_server().get_role_mgr().get_state())
            {
              TBSYS_LOG(WARN, "invalid role mgr stat=%d", ups_main->get_update_server().get_role_mgr().get_state());
              tmp_ret = OB_ERROR;
              break;
            }
          }
          iter.reset_iter();
          int64_t trailer_offset = 0;
          int64_t sstable_size = 0;
          if (OB_ITER_END != tmp_ret)
          {
            TBSYS_LOG(WARN, "iterate row fail ret=%d", tmp_ret);
          }
          else if (OB_SUCCESS != (tmp_ret = sstable_writer.close_sstable(trailer_offset, sstable_size)))
          {
            TBSYS_LOG(WARN, "close sstable fail ret=%d", tmp_ret);
          }
          else
          {
            TBSYS_LOG(INFO, "build sstable succ trailer_offset=%ld sstable_size=%ld", trailer_offset, sstable_size);
            bret = true;
          }
        }
      }
      return bret;
    }

    bool SSTableMgr::build_multi_sstable_file_(const ObList<StoreMgr::Handle> &store_list,
                                              SSTableInfo &sstable_info, const int64_t time_stamp, IRowIterator &iter)
    {
      bool bret = false;
      static const int64_t fpaths_size = 512 * 1024;
      char *fpaths = NULL;
      if (NULL == (fpaths = (char*)ob_malloc(fpaths_size)))
      {
        TBSYS_LOG(WARN, "malloc fpaths fail");
      }
      else
      {
        memset(fpaths, 0, sizeof(fpaths_size));
        StoreMgr::Handle store_handle = StoreMgr::INVALID_HANDLE;
        ObList<StoreMgr::Handle>::const_iterator list_iter;
        int64_t pos = 0;
        const char *path = NULL;
        const char *fname_substr = NULL;
        const char *fpath_tmp = NULL;
        const char *fpath = NULL;
        int64_t timestamp = tbsys::CTimeUtil::getTime();
        int64_t buffer_pos = 0;
        int64_t buffer_size = fpaths_size;
        int64_t str_ret = 0;
        for (list_iter = store_list.begin(); list_iter != store_list.end(); list_iter++, pos++)
        {
          store_handle = *list_iter;
          path = store_mgr_.get_dir(store_handle);
          fname_substr = sstable_id2str(sstable_info.get_sstable_id(), sstable_info.get_clog_id());
          fpath_tmp = NULL;
          if (NULL != path || NULL != fname_substr)
          {
            fpath_tmp = build_str("%s/%s%s.%ld.tmp", path, fname_substr, SSTABLE_SUFFIX, timestamp);
          }
          if (NULL != fpath_tmp)
          {
            str_ret = snprintf(fpaths + buffer_pos, buffer_size, "%s%c", fpath_tmp, '\0');
            if (buffer_size <= str_ret)
            {
              break;
            }
            TBSYS_LOG(INFO, "build fpath succ [%s]", fpath_tmp);
            buffer_pos += str_ret;
            buffer_size -= str_ret;
          }
        }
        if (0 < pos)
        {
          ObString fpath_str;
          fpath_str.assign_ptr(fpaths, buffer_pos + 1);
          if (!build_sstable_file_(sstable_info.get_sstable_id(), fpath_str, time_stamp, iter))
          {
            TBSYS_LOG(WARN, "build sstable file fail sstable_id=%lu clog_id=%lu",
                      sstable_info.get_sstable_id(), sstable_info.get_clog_id());
            pos = 0;
          }
        }
        int64_t i = 0;
        for (list_iter = store_list.begin(); i < pos && list_iter != store_list.end(); list_iter++, i++)
        {
          store_handle = *list_iter;
          path = store_mgr_.get_dir(store_handle);
          fname_substr = sstable_id2str(sstable_info.get_sstable_id(), sstable_info.get_clog_id());
          fpath_tmp = NULL;
          if (NULL != path || NULL != fname_substr)
          {
            fpath_tmp = build_str("%s/%s%s.%ld.tmp", path, fname_substr, SSTABLE_SUFFIX, timestamp);
            fpath = build_str("%s/%s%s", path, fname_substr, SSTABLE_SUFFIX);
          }
          if (NULL != fpath_tmp && NULL != fpath)
          {
            if (0 != rename(fpath_tmp, fpath))
            {
              TBSYS_LOG(WARN, "rename [%s] to [%s] fail errno=%u", fpath_tmp, fpath, errno);
            }
            else
            {
              TBSYS_LOG(INFO, "rename [%s] to [%s] succ", fpath_tmp, fpath);
              sstable_info.add_store(store_handle);
            }
          }
        }
        if (0 >= sstable_info.get_store_num())
        {
          TBSYS_LOG(WARN, "no valid sstable file has been build sstable_id=%lu clog_id=%lu",
                    sstable_info.get_sstable_id(), sstable_info.get_clog_id());
        }
        else
        {
          bret = true;
        }
      }
      if (NULL != fpaths)
      {
        ob_free(fpaths);
      }
      return bret;
    }

    int SSTableMgr::add_sstable(const uint64_t sstable_id, const uint64_t clog_id, const int64_t time_stamp, IRowIterator &iter)
    {
      int ret = OB_SUCCESS;
      SSTableInfo *sstable_info = NULL;
      ObList<StoreMgr::Handle> store_list;
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
        ret = OB_NOT_INIT;
      }
      else if (OB_INVALID_ID == sstable_id
              || OB_INVALID_ID == clog_id)
      {
        TBSYS_LOG(WARN, "invalid param sstable_id=%lu clog_id=%lu", sstable_id, clog_id);
        ret = OB_INVALID_ARGUMENT;
      }
      else if (NULL == (sstable_info = sstable_allocator_.allocate()))
      {
        TBSYS_LOG(WARN, "allocate sstable_info fail sstable_id=%lu", sstable_id);
        ret = OB_ERROR;
      }
      else if (OB_SUCCESS != (ret = sstable_info->init(this, sstable_id, clog_id)))
      {
        TBSYS_LOG(WARN, "init sstable_info fail ret=%d sstable_id=%lu", ret, sstable_id);
      }
      else if (OB_SUCCESS != (ret = store_mgr_.assign_stores(store_list)))
      {
        TBSYS_LOG(WARN, "assign_stores fail ret=%d sstable_id=%lu", ret, sstable_id);
      }
      else
      {
        if (sstable_exist_(sstable_id))
        {
          SSTableID sst_id = sstable_id;
          TBSYS_LOG(WARN, "%s has already exist, will erase it", sst_id.log_str());
          bool remove_sstable_file = true;
          erase_sstable(sstable_id, remove_sstable_file);
        }
        if (!build_multi_sstable_file_(store_list, *sstable_info, time_stamp, iter))
        {
          TBSYS_LOG(WARN, "no valid sstable file has been build sstable_id=%lu", sstable_id);
          ret = OB_ERROR;
        }
        else
        {
          map_lock_.wrlock();
          int hash_ret = sstable_info_map_.set(sstable_id, sstable_info);
          map_lock_.unlock();
          if (hash::HASH_INSERT_SUCC != hash_ret)
          {
            TBSYS_LOG(WARN, "set to sstable_info_map fail ret=%d sstable_id=%lu", hash_ret, sstable_id);
            ret = OB_ERROR;
          }
          else
          {
            TBSYS_LOG(INFO, "dump sstable succ sstable_id=%lu", sstable_id);
          }
        }
      }
      if (OB_SUCCESS != ret
          && NULL != sstable_info)
      {
        sstable_allocator_.deallocate(sstable_info);
      }
      return ret;
    }

    int SSTableMgr::erase_sstable(const uint64_t sstable_id, const bool remove_sstable_file)
    {
      int ret = OB_SUCCESS;
      SSTableInfo *sstable_info = NULL;
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else
      {
        map_lock_.wrlock();
        int hash_ret = sstable_info_map_.erase(sstable_id, &sstable_info);
        map_lock_.unlock();
        if (hash::HASH_EXIST != hash_ret)
        {
          TBSYS_LOG(WARN, "erase sstable_info fail ret=%d sstable_id=%lu", hash_ret, sstable_id);
          ret = OB_ERROR;
        }
        else
        {
          if (remove_sstable_file)
          {
            sstable_info->remove_sstable_file();
          }
          if (0 == sstable_info->dec_ref_cnt())
          {
            sstable_allocator_.deallocate(sstable_info);
            TBSYS_LOG(INFO, "erase sstable succ sstable_id=%lu", sstable_id);
          }
        }
      }
      return ret;
    }

    void SSTableMgr::check_broken()
    {
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else
      {
        ObList<uint64_t> deleted_list;
        ObList<uint64_t>::iterator list_iter;
        StoreMgr::Handle store_handle = StoreMgr::INVALID_HANDLE;
        int64_t timeu = tbsys::CTimeUtil::getTime();
        TBSYS_LOG(INFO, "start check broken");
        while (NULL != store_mgr_.get_broken_dir(store_handle))
        {
          SSTableInfoMap::iterator iter;
          map_lock_.wrlock();
          for (iter = sstable_info_map_.begin(); iter != sstable_info_map_.end();)
          {
            uint64_t sstable_id = iter->first;
            SSTableInfo *sstable_info = iter->second;
            ++iter;
            if (NULL != sstable_info)
            {
              sstable_info->erase_store(store_handle);
              if (0 == sstable_info->get_store_num()
                  && 0 == sstable_info->dec_ref_cnt())
              {
                sstable_info_map_.erase(sstable_id);
                deleted_list.push_back(sstable_id);
                sstable_allocator_.deallocate(sstable_info);
              }
            }
          }
          map_lock_.unlock();
          store_mgr_.umount(store_handle);
        }
        TBSYS_LOG(INFO, "end check broken erase timeu=%ld erase_sstable_num=%ld",
                  tbsys::CTimeUtil::getTime() - timeu, deleted_list.size());
        for (list_iter = deleted_list.begin(); list_iter != deleted_list.end(); list_iter++)
        {
          erase_sstable_callback_(*list_iter);
        }
      }
    }

    void SSTableMgr::reload(const StoreMgr::Handle store_handle)
    {
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else
      {
        load_dir_(store_handle);
      }
    }

    void SSTableMgr::reload_all()
    {
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else
      {
        int64_t timeu = tbsys::CTimeUtil::getTime();
        ObList<StoreMgr::Handle> store_list;
        if (OB_SUCCESS == store_mgr_.get_all_valid_stores(store_list))
        {
          ObList<StoreMgr::Handle>::iterator iter;
          for (iter = store_list.begin(); iter != store_list.end(); iter++)
          {
            load_dir_(*iter);
          }
        }
        TBSYS_LOG(INFO, "reload all store_num=%ld timeu=%ld", store_list.size(), tbsys::CTimeUtil::getTime() - timeu);
      }
    }

    bool SSTableMgr::load_new()
    {
      bool bret = false;
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else
      {
        bret = store_mgr_.load();
        StoreMgr::Handle store_handle = StoreMgr::INVALID_HANDLE;
        const char *path = NULL;
        int64_t timeu = tbsys::CTimeUtil::getTime();
        int64_t sstable_num = sstable_info_map_.size();
        TBSYS_LOG(INFO, "start load");
        while (NULL != (path = store_mgr_.get_new_dir(store_handle)))
        {
          load_dir_(store_handle);
        }
        TBSYS_LOG(INFO, "end load timeu=%ld add_sstable_num=%ld bret=%d",
                  tbsys::CTimeUtil::getTime() - timeu, sstable_info_map_.size() - sstable_num, bret);
      }
      return bret;
    }

    void SSTableMgr::umount_store(const char *path)
    {
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else if (NULL == path)
      {
        TBSYS_LOG(WARN, "invalid param path=%p", path);
      }
      else
      {
        store_mgr_.report_broken(path);
      }
    }

    int SSTableMgr::reg_observer(ISSTableObserver *observer)
    {
      int ret = OB_SUCCESS;
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
        ret = OB_NOT_INIT;
      }
      else if (NULL == observer)
      {
        TBSYS_LOG(WARN, "invalid param observer=%p", observer);
        ret = OB_INVALID_ARGUMENT;
      }
      else if (OB_SUCCESS != (ret = observer_list_.push_back(observer)))
      {
        TBSYS_LOG(WARN, "push observer to list fail ret=%d observer=%p", ret, observer);
      }
      else
      {
        // do nothing
      }
      return ret;
    }

    int SSTableMgr::fill_fetch_param(const uint64_t min_sstable_id, const uint64_t max_sstable_id,
                                     const uint64_t max_fill_major_num, ObUpsFetchParam &fetch_param)
    {
      int ret = OB_SUCCESS;
      char *cur_dir = get_current_dir_name();
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
        ret = OB_NOT_INIT;
      }
      else if (NULL == cur_dir)
      {
        TBSYS_LOG(WARN, "get current dir name failed errno=%u", errno);
        ret = OB_ERROR;
      }
      else
      {
        SSTableID slave_start = min_sstable_id;
        SSTableID slave_end = max_sstable_id;
        SSTableID master_start = get_min_sstable_id();
        SSTableID master_end = get_max_sstable_id();
        SSTableID range_start = master_start;
        SSTableID range_end = master_end;
        if ((range_start.major_version + max_fill_major_num) <= range_end.major_version)
        {
          range_start.major_version = range_end.major_version - max_fill_major_num + 1;
          range_start.minor_version_start = 0;
          range_start.minor_version_end = 0;
        }
        
        ObList<SSTableInfo*> list2fill;
        SSTableInfoMap::iterator iter;
        map_lock_.rdlock();
        SSTableID tmp_sst_id = 0;
        for (iter = sstable_info_map_.begin(); iter != sstable_info_map_.end(); iter++)
        {
          SSTableInfo *sstable_info = iter->second;
          if (NULL == sstable_info)
          {
            TBSYS_LOG(ERROR, "invalid sstable_info=%p", sstable_info);
            ret = OB_ERROR;
            break;
          }

          tmp_sst_id = sstable_info->get_sstable_id();
          if (0 >= SSTableID::compare(range_start, tmp_sst_id)
              && 0 <= SSTableID::compare(range_end, tmp_sst_id))
          {
            if (OB_SUCCESS != (ret = list2fill.push_back(sstable_info)))
            {
              TBSYS_LOG(ERROR, "add sstable_info to list fail ret=%d %s", ret, tmp_sst_id.log_str());
              break;
            }
          }
        }

        ObList<SSTableInfo*>::iterator list_iter;
        int64_t add_succ_num = 0;
        for (list_iter = list2fill.begin(); list_iter != list2fill.end(); list_iter++)
        {
          SSTableInfo *sstable_info = *list_iter;
          StoreInfo *store_info = NULL;
          if (NULL == sstable_info
              || NULL == (store_info = sstable_info->get_store_info()))
          {
            TBSYS_LOG(ERROR, "invalid sstable_info=%p or store_info=%p", sstable_info, store_info);
            ret = OB_ERROR;
            break;
          }

          tmp_sst_id = sstable_info->get_sstable_id();
          if (0 > SSTableID::compare(slave_end, tmp_sst_id))
          {
            SSTFileInfo sst_file_info;
            const char *path = store_info->get_dir();
            if (NULL != path
                && '/' != path[0])
            {
              path = build_str("%s/%s", cur_dir, path);
            }
            const char *name = build_str("%s%s", sstable_info->get_fname_substr(), SSTABLE_SUFFIX);
            sst_file_info.path.assign_ptr(const_cast<char*>(path), (NULL == path) ? 0 : strlen(path) + 1);
            sst_file_info.name.assign_ptr(const_cast<char*>(name), (NULL == name) ? 0 : strlen(name) + 1);
            if (NULL == path || NULL == name
                || OB_SUCCESS != (ret = fetch_param.add_sst_file_info(sst_file_info)))
            {
              TBSYS_LOG(ERROR, "add sstable_info to list fail ret=%d %s", ret, tmp_sst_id.log_str());
              break;
            }
            else
            {
              TBSYS_LOG(INFO, "add sstable_info to fetch param succ %s fpath=%s/%s",
                        tmp_sst_id.log_str(), path, name);
              add_succ_num += 1;
            }
          }
        }
        if (OB_SUCCESS == ret && 0 != add_succ_num)
        {
          fetch_param.fetch_sstable_ = true;
        }
        uint64_t log_id = get_max_clog_id();
        if (OB_INVALID_ID != log_id)
        {
          fetch_param.min_log_id_ = log_id;
        }

        map_lock_.unlock();

      }
      if (NULL != cur_dir)
      {
        ::free(cur_dir);
        cur_dir = NULL;
      }
      return ret;
    }

    void SSTableMgr::load_dir_(const StoreMgr::Handle store_handle)
    {
      const char *path = store_mgr_.get_dir(store_handle);
      if (NULL != path) {
        DIR *dd = opendir(path);
        if (NULL != dd)
        {
          int64_t timeu = tbsys::CTimeUtil::getTime();
          struct dirent* dir_iter = NULL;
          while (NULL != (dir_iter = readdir(dd))
                && NULL != dir_iter->d_name)
          {
            uint64_t sstable_id = OB_INVALID_ID;
            uint64_t clog_id = OB_INVALID_ID;
            if (!sstable_fname_regex_.match(dir_iter->d_name, 0))
            {
              TBSYS_LOG(DEBUG, "fname=[%s/%s] do not match regex", path, dir_iter->d_name);
              continue;
            }
            const char *fpath = build_str("%s/%s", path, dir_iter->d_name);
            if (!StoreMgr::is_access(fpath)
                || !StoreMgr::is_rfile(fpath))
            {
              continue;
            }
            if (!sstable_str2id(dir_iter->d_name, sstable_id, clog_id)
                || OB_INVALID_ID == sstable_id
                || OB_INVALID_ID == clog_id)
            {
              TBSYS_LOG(WARN, "fname=[%s/%s] trans to sstable_id fail", path, dir_iter->d_name);
              continue;
            }
            if (!check_sstable_(dir_iter->d_name))
            {
              TBSYS_LOG(WARN, "fpath=[%s] do not pass sstable check, will not load it", fpath);
              continue;
            }
            add_sstable_file_(sstable_id, clog_id, store_handle);
          }
          closedir(dd);
          TBSYS_LOG(INFO, "load dir path=[%s] store_handle=%lu timeu=%ld",
                    path, store_handle, tbsys::CTimeUtil::getTime() - timeu);
        }
      }
    }

    bool SSTableMgr::sstable_exist_(const uint64_t sstable_id)
    {
      bool bret = false;
      SSTableInfo *sstable_info = NULL;
      map_lock_.rdlock();
      bret = (hash::HASH_EXIST == sstable_info_map_.get(sstable_id, sstable_info));
      map_lock_.unlock();
      return bret;
    }

    bool SSTableMgr::check_sstable_(const char *fpath)
    {
      bool bret = false;
      //TODO
      UNUSED(fpath);
      bret = true;
      return bret;
    }

    void SSTableMgr::add_sstable_file_(const uint64_t sstable_id, const uint64_t clog_id, const StoreMgr::Handle store_handle)
    {
      bool add_succ = false;
      SSTableInfo *sstable_info = NULL;
      map_lock_.wrlock();
      int hash_ret = sstable_info_map_.get(sstable_id, sstable_info);
      if (hash::HASH_EXIST == hash_ret
          && NULL != sstable_info)
      {
        sstable_info->add_store(store_handle);
      }
      else if (hash::HASH_NOT_EXIST == hash_ret)
      {
        int tmp_ret = OB_SUCCESS;
        if (NULL == (sstable_info = sstable_allocator_.allocate()))
        {
          TBSYS_LOG(WARN, "allocate sstable_info fail sstable_id=%lu fname=[%s] store_handle=%lu",
                    sstable_id, sstable_id2str(sstable_id, clog_id), store_handle);
        }
        else if (OB_SUCCESS != (tmp_ret = sstable_info->init(this, sstable_id, clog_id)))
        {
          TBSYS_LOG(WARN, "init sstable_info fail ret=%d sstable_id=%lu fname=[%s] store_handle=%lu",
                    tmp_ret, sstable_id, sstable_id2str(sstable_id, clog_id), store_handle);
        }
        else if (OB_SUCCESS != (tmp_ret = sstable_info->add_store(store_handle)))
        {
          TBSYS_LOG(WARN, "add_store to sstable_info fail ret=%d sstable_id=%lu fname=[%s] store_handle=%lu",
                    tmp_ret, sstable_id, sstable_id2str(sstable_id, clog_id), store_handle);
        }
        else if (hash::HASH_INSERT_SUCC != (hash_ret = sstable_info_map_.set(sstable_id, sstable_info)))
        {
          TBSYS_LOG(WARN, "add sstable_info to map fail hash_ret=%d sstable_id=%lu fname=[%s] store_handle=%lu,",
                    hash_ret, sstable_id, sstable_id2str(sstable_id, clog_id), store_handle);
        }
        else
        {
          add_succ = true;
        }
        if (!add_succ
            && NULL != sstable_info)
        {
          sstable_allocator_.deallocate(sstable_info);
        }
      }
      map_lock_.unlock();
      if (add_succ)
      {
        add_sstable_callback_(sstable_id);
      }
    }

    void SSTableMgr::add_sstable_callback_(const uint64_t sstable_id)
    {
      ObserverList::iterator iter;
      for (iter = observer_list_.begin(); iter != observer_list_.end(); iter++)
      {
        ISSTableObserver *cb = *iter;
        if (NULL != cb)
        {
          if (OB_SUCCESS != cb->add_sstable(sstable_id))
          {
            bool remove_sstable_file = false;
            erase_sstable(sstable_id, remove_sstable_file);
          }
        }
      }
    }

    void SSTableMgr::erase_sstable_callback_(const uint64_t sstable_id)
    {
      ObserverList::iterator iter;
      for (iter = observer_list_.begin(); iter != observer_list_.end(); iter++)
      {
        ISSTableObserver *cb = *iter;
        if (NULL != cb)
        {
          cb->erase_sstable(sstable_id);
        }
      }
    }

    uint64_t SSTableMgr::get_min_sstable_id()
    {
      uint64_t ret = UINT64_MAX;
      SSTableInfoMap::iterator iter;
      map_lock_.rdlock();
      for (iter = sstable_info_map_.begin(); iter != sstable_info_map_.end(); iter++)
      {
        SSTableInfo *sstable_info = iter->second;
        if (NULL != sstable_info)
        {
          uint64_t sstable_id = sstable_info->get_sstable_id();
          ret = 0 > SSTableID::compare(ret, sstable_id) ? ret : sstable_id;
        }
      }
      map_lock_.unlock();
      return ret;
    }

    uint64_t SSTableMgr::get_max_sstable_id()
    {
      uint64_t ret = 0;
      SSTableInfoMap::iterator iter;
      map_lock_.rdlock();
      for (iter = sstable_info_map_.begin(); iter != sstable_info_map_.end(); iter++)
      {
        SSTableInfo *sstable_info = iter->second;
        if (NULL != sstable_info)
        {
          uint64_t sstable_id = sstable_info->get_sstable_id();
          ret = 0 < SSTableID::compare(ret, sstable_id) ? ret : sstable_id;
        }
      }
      map_lock_.unlock();
      return ret;
    }

    uint64_t SSTableMgr::get_max_clog_id()
    {
      uint64_t ret = 0;
      SSTableInfoMap::iterator iter;
      map_lock_.rdlock();
      for (iter = sstable_info_map_.begin(); iter != sstable_info_map_.end(); iter++)
      {
        SSTableInfo *sstable_info = iter->second;
        if (NULL != sstable_info)
        {
          ret = std::max(ret, sstable_info->get_clog_id());
        }
      }
      map_lock_.unlock();
      ret = (0 == ret) ? OB_INVALID_ID : ret;
      return ret;
    }

    const char *SSTableMgr::build_str(const char *fmt, ...)
    {
      char *ret = NULL;
      static __thread char fpaths[2][StoreMgr::MAX_DIR_NAME_LENGTH];
      static __thread int64_t i = 0;
      char *fpath = fpaths[i++ % 2];
      va_list args;
      va_start(args, fmt);
      if (StoreMgr::MAX_DIR_NAME_LENGTH > vsnprintf(fpath, StoreMgr::MAX_DIR_NAME_LENGTH, fmt, args))
      {
        ret = fpath;
      }
      va_end(args);
      return ret;
    }

    const char *SSTableMgr::sstable_id2str(const uint64_t sstable_id, const uint64_t clog_id)
    {
      char *ret = NULL;
      static __thread char str[SSTableInfo::MAX_SUBSTR_SIZE];
      SSTableID sst_id(sstable_id);
      if (SSTableInfo::MAX_SUBSTR_SIZE > snprintf(str, SSTableInfo::MAX_SUBSTR_SIZE, "%lu_%lu-%lu_%lu",
                                                  sst_id.major_version, sst_id.minor_version_start,
                                                  sst_id.minor_version_end, clog_id))
      {
        ret = str;
      }
      return ret;
    }

    bool SSTableMgr::sstable_str2id(const char *sstable_str, uint64_t &sstable_id, uint64_t &clog_id)
    {
      bool bret = false;
      if (NULL != sstable_str)
      {
        char *next = NULL;
        const int64_t num = 4;
        uint64_t result[num];
        uint64_t check_overflow[num] = {SSTableID::MAX_MAJOR_VERSION, SSTableID::MAX_MINOR_VERSION,
                                      SSTableID::MAX_MINOR_VERSION, SSTableID::MAX_CLOG_ID};
        bret = true;
        for (int64_t i = 0; i < num && bret; i++)
        {
          errno = 0;
          result[i] = strtoll(sstable_str, &next, 10);
          if (0 != errno
              || check_overflow[i] <= result[i])
          {
            bret = false;
            break;
          }
          else
          {
            while (next && *next)
            {
              if (isdigit(*next)) 
              {
                break;
              }
              next++;
            }
            sstable_str = next;
          }
        }
        if (bret)
        {
          SSTableID sst_id;
          sst_id.major_version = result[0];
          sst_id.minor_version_start = result[1];
          sst_id.minor_version_end = result[2];
          sstable_id = sst_id.id;
          clog_id = result[3];
        }
      }
      return bret;
    }

    const IFileInfo *SSTableMgr::get_fileinfo(const uint64_t sstable_id)
    {
      StoreInfo *ret = NULL;
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else if (OB_SUCCESS != map_lock_.rdlock())
      {
        TBSYS_LOG(WARN, "map_lock rdlock fail");
      }
      else
      {
        SSTableInfo *sstable_info = NULL;
        if (hash::HASH_EXIST != sstable_info_map_.get(sstable_id, sstable_info)
            || NULL == sstable_info)
        {
          TBSYS_LOG(WARN, "get from sstable_info_map fail sstable_id=%lu", sstable_id);
        }
        else
        {
          StoreInfo *store_info = sstable_info->get_store_info();
          if (NULL != store_info)
          {
            sstable_info->inc_ref_cnt();
            store_info->inc_ref_cnt();
            ret = store_info;
          }
        }
        map_lock_.unlock();
      }
      return ret;
    }

    int SSTableMgr::revert_fileinfo(const IFileInfo *file_info)
    {
      int ret = OB_SUCCESS;
      StoreInfo *store_info = const_cast<StoreInfo*>(dynamic_cast<const StoreInfo*>(file_info));
      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
      }
      else if (NULL == store_info)
      {
        TBSYS_LOG(WARN, "invalid param file_info=%p", file_info);
        ret = OB_INVALID_ARGUMENT;
      }
      else if (OB_SUCCESS != map_lock_.rdlock())
      {
        TBSYS_LOG(WARN, "map_lock rdlock fail");
      }
      else
      {
        SSTableInfo *sstable_info = store_info->get_sstable_info();
        if (0 == store_info->dec_ref_cnt())
        {
          store_allocator_.deallocate(store_info);
        }
        if (0 == sstable_info->dec_ref_cnt())
        {
          sstable_allocator_.deallocate(sstable_info);
        }
        map_lock_.unlock();
      }
      return ret;
    }

    void SSTableMgr::log_sstable_info()
    {
      SSTableInfoMap::iterator iter;
      TBSYS_LOG(INFO, "==========log sstable info start==========");
      map_lock_.rdlock();
      for (iter = sstable_info_map_.begin(); iter != sstable_info_map_.end(); iter++)
      {
        SSTableInfo *sstable_info = iter->second;
        if (NULL != sstable_info)
        {
          sstable_info->log_sstable_info();
        }
      }
      map_lock_.unlock();
      TBSYS_LOG(INFO, "==========log sstable info end==========");
    }
  }
}



