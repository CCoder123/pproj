/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * Version: 5567
 *
 * ob_block_index_cache.cpp
 *
 * Authors:
 *     huating <huating.zmq@taobao.com>
 *
 */
#include <tbsys.h>
#include <tblog.h>
#include "common/ob_malloc.h"
#include "common/ob_file.h"
#include "ob_block_index_cache.h"
#include "ob_disk_path.h"
#include "ob_sstable_stat.h"

namespace oceanbase
{
  namespace sstable
  {
    using namespace tbsys;
    using namespace oceanbase::common;

    ObBlockIndexCache::ObBlockIndexCache()
    : inited_(false), fileinfo_cache_(NULL)
    {

    }

    ObBlockIndexCache::ObBlockIndexCache(IFileInfoMgr& fileinfo_cache)
    : inited_(false), fileinfo_cache_(&fileinfo_cache)
    {

    }

    ObBlockIndexCache::~ObBlockIndexCache()
    {

    }
    
    int ObBlockIndexCache::init(const ObBlockIndexCacheConf& conf)
    {
      int ret = OB_SUCCESS;

      if (inited_)
      {
        TBSYS_LOG(INFO, "have inited");
        ret = OB_ERROR;
      }
      else if (OB_SUCCESS != kv_cache_.init(conf.cache_mem_size))
      {
        TBSYS_LOG(WARN, "init kv cache fail");
        ret = OB_ERROR;
      }
      else
      {
        inited_ = true;
        TBSYS_LOG_US(DEBUG, "init block index cache succ, cache_mem_size=%ld,",
                     conf.cache_mem_size);
      }

      return ret;
    }

    int ObBlockIndexCache::read_record(IFileInfoMgr& fileinfo_cache, 
                                       const uint64_t sstable_id, 
                                       const int64_t offset, 
                                       const int64_t size, 
                                       const char*& out_buffer)
    {
      int ret                 = OB_SUCCESS;
      ObFileBuffer* file_buf  = GET_TSI(ObFileBuffer);
      out_buffer = NULL;

      if (NULL == file_buf)
      {
        TBSYS_LOG(WARN, "get thread file read buffer failed, file_buf=NULL");
        ret = OB_ERROR;
      }
      else
      {
        ret = ObFileReader::read_record(fileinfo_cache, sstable_id, offset, 
                                        size, *file_buf);
        if (OB_SUCCESS == ret)
        {
          out_buffer = file_buf->get_buffer() + file_buf->get_base_pos();
        }
      }

      return ret;
    }
    
    int ObBlockIndexCache::read_sstable_block_index(
        const ObBlockIndexPositionInfo& block_index_info,
        ObSSTableBlockIndexV2& block_index, 
        const uint64_t table_id,
        Handle& handle)
    {
      int ret                         = OB_SUCCESS;
      int64_t block_index_size        = sizeof(ObSSTableBlockIndexV2);
      int64_t read_size               = 0;
      int64_t read_offset             = 0;
      ObSSTableBlockIndexV2* bci_tmp  = NULL;
      const char* record_buf          = NULL;
    
      if (!inited_ || NULL == fileinfo_cache_)
      {
        TBSYS_LOG(WARN, "have not inited, fileinfo_cache_=%p", fileinfo_cache_);
        ret = OB_ERROR;
      }
      else if (OB_INVALID_ID == table_id || 0 == table_id)
      {
        TBSYS_LOG(WARN, "invalid table_id=%lu", table_id);
        ret = OB_ERROR;
      }
      else
      {
        /**
         * read the extram sizeof(ObSSTableBlockIndexV2) data to occupy 
         * the space for ObSSTableBlockIndexV2 instance. 
         */
        read_size = block_index_size + block_index_info.size_;
        if (block_index_info.offset_ >= block_index_size)
        {
          read_offset = block_index_info.offset_ - block_index_size;
        }
        else
        {
          /**
           * FIXME: we assumpt that the data after block index is larger 
           * than sizeof(ObSSTableBlockIndexV2), so read the extra data 
           * can't reach the end of file 
           */
          read_offset = 0;
        }
      }

      if (OB_SUCCESS == ret)
      {
        //read block index data from sstable file
        ret = read_record(*fileinfo_cache_, block_index_info.sstable_file_id_, 
                          read_offset, read_size, record_buf);
        if (OB_SUCCESS != ret || NULL == record_buf)
        {
          TBSYS_LOG(WARN, "pread block index from sstable failed, sstable_id=%lu, "
                          "offset=%ld, nbyte=%ld, record_buf=%p",
                    block_index_info.sstable_file_id_, block_index_info.offset_, 
                    block_index_info.size_, record_buf);
        }
        else
        {
          INC_STAT(table_id, INDEX_DISK_IO_NUM, 1);
          INC_STAT(table_id, INDEX_DISK_IO_BYTES, read_size);
        }
      }

      if (OB_SUCCESS == ret)
      {
        if (block_index_info.offset_ < block_index_size)
        {
          //ensure the space is enough to store one ObSSTableBlockIndexV2 instance
          memmove(const_cast<char*>(record_buf + block_index_size), 
                  const_cast<char*>(record_buf + block_index_info.offset_),
                  block_index_info.size_);
        }
        //create ObSSTableBlockIndexV2 instance
        bci_tmp = new (const_cast<char*>(record_buf)) 
                    ObSSTableBlockIndexV2(block_index_info.size_, false);
        if (NULL == bci_tmp)
        {
          TBSYS_LOG(WARN, "placement new block index instance failed");
          ret = OB_ERROR;
        }
        else
        {
          ret = kv_cache_.put_and_fetch(block_index_info, *bci_tmp, block_index, 
                                        handle, false, false);
          if (OB_SUCCESS != ret)
          {
            TBSYS_LOG(WARN, "failed to put and fetch block index from kvcache, table_id=%lu",
                      table_id);
          }
        }
      }

      return ret;
    }
    
    int ObBlockIndexCache::load_block_index(
        const ObBlockIndexPositionInfo& block_index_info,
        ObSSTableBlockIndexV2& block_index,
        const uint64_t table_id,
        Handle& handle)
    {
      int ret = OB_SUCCESS;

      if (!inited_ || NULL == fileinfo_cache_)
      {
        TBSYS_LOG(WARN, "have not inited, fileinfo_cache_=%p", fileinfo_cache_);
        ret = OB_ERROR;
      }
      else
      {
        //look up block cache to check whether the current block index is in cahce
        ret = kv_cache_.get(block_index_info, block_index, handle, false);
        if (OB_SUCCESS == ret)
        {
          INC_STAT(table_id, INDEX_BLOCK_INDEX_CACHE_HIT, 1);
        }
        else
        {
          //read data from disk
          ret = read_sstable_block_index(block_index_info, block_index, table_id, handle);
          INC_STAT(table_id, INDEX_BLOCK_INDEX_CACHE_MISS, 1);
        }
      }
      
      return ret;
    }

    int ObBlockIndexCache::check_param(
        const ObBlockIndexPositionInfo& block_index_info,
        const uint64_t table_id,
        const uint64_t column_group_id)
    {
      int ret = OB_SUCCESS;

      if (!inited_ || NULL == fileinfo_cache_)
      {
        TBSYS_LOG(WARN, "have not inited, fileinfo_cache_=%p", fileinfo_cache_);
        ret = OB_ERROR;
      }
      else if (OB_INVALID_ID == block_index_info.sstable_file_id_ 
               || block_index_info.offset_ < 0 || block_index_info.size_ <= 0
               || OB_INVALID_ID == table_id || 0 == table_id
               || OB_INVALID_ID == column_group_id)
      {
        TBSYS_LOG(WARN, "invalid param sstable_id=%lu, offset=%ld, nbyte=%ld, "
                        "table_id=%lu, column_group_id=%lu", 
                  block_index_info.sstable_file_id_, block_index_info.offset_, 
                  block_index_info.size_, table_id, column_group_id);
        ret = OB_ERROR;
      }

      return ret;
    }
    
    int ObBlockIndexCache::get_block_position_info(
        const ObBlockIndexPositionInfo& block_index_info,
        const uint64_t table_id,
        const uint64_t column_group_id,
        const ObString& key,
        const SearchMode search_mode,
        ObBlockPositionInfos& pos_info)
    {
      int ret = OB_SUCCESS;
      bool revert_handle = false;
      ObSSTableBlockIndexV2 block_index;
      Handle handle;
    
      if (OB_SUCCESS != (ret = 
            check_param(block_index_info, table_id, column_group_id)))
      {
        TBSYS_LOG(ERROR, "check_param error, table_id=%ld, column_group_id=%ld",
            table_id, column_group_id);
      }
      else if ( is_regular_mode(search_mode)
          && (NULL == key.ptr() || key.length() <= 0) )
      {
        TBSYS_LOG(WARN, "invalid param, key_ptr=%p, key_len=%d",
                  key.ptr(), key.length());
        ret = OB_INVALID_ARGUMENT;
      }
      else if ( OB_SUCCESS != (ret =
          load_block_index(block_index_info, block_index, table_id, handle)) )
      {
        TBSYS_LOG(ERROR, "load block index error, ret=%d, table_id=%ld"
            "column_group_id=%ld, mode=%d, pos=%ld", 
            ret, table_id, column_group_id, search_mode, pos_info.block_count_);
      }
      else
      {
        revert_handle = true;
        ret = block_index.search_batch_blocks_by_key(
            table_id, column_group_id, key, search_mode, pos_info);
      }
      
      if (revert_handle && OB_SUCCESS != kv_cache_.revert(handle))
      {
        //must revert the handle
        TBSYS_LOG(WARN, "failed to revert  block index cache handle");
      }

      return ret;
    }

    int ObBlockIndexCache::get_block_position_info(
        const ObBlockIndexPositionInfo& block_index_info,
        const uint64_t table_id,
        const uint64_t column_group_id,
        const common::ObRange& range,
        const bool is_reverse_scan,
        ObBlockPositionInfos& pos_info)
    {
      int ret = OB_SUCCESS;
      bool revert_handle = false;
      ObSSTableBlockIndexV2 block_index;
      Handle handle;
    
      if ( OB_SUCCESS != (ret = 
            check_param(block_index_info, table_id, column_group_id)) )
      {
        TBSYS_LOG(ERROR, "check_param error, table_id=%ld, column_group_id=%ld",
            table_id, column_group_id);
      }
      else if ( OB_SUCCESS != (ret =
          load_block_index(block_index_info, block_index, table_id, handle)) )
      {
        TBSYS_LOG(ERROR, "load block index error, ret=%d, table_id=%ld"
            "column_group_id=%ld, is_reverse_scan=%d, pos=%ld", 
            ret, table_id, column_group_id, is_reverse_scan, pos_info.block_count_);
      }
      else
      {
        revert_handle = true;
        ret = block_index.search_batch_blocks_by_range(
            table_id, column_group_id, range, is_reverse_scan, pos_info);
      }

      if (revert_handle && OB_SUCCESS != kv_cache_.revert(handle))
      {
        //must revert the handle
        TBSYS_LOG(WARN, "failed to revert  block index cache handle");
      }

      return ret;
    }

    int ObBlockIndexCache::get_single_block_pos_info(
        const ObBlockIndexPositionInfo& block_index_info,
        const uint64_t table_id,
        const uint64_t column_group_id,
        const ObString& key,
        const SearchMode search_mode,
        ObBlockPositionInfo& pos_info)
    {
      int ret = OB_SUCCESS;
      bool revert_handle = false;
      ObSSTableBlockIndexV2 block_index;
      Handle handle;

      if ( OB_SUCCESS != (ret = 
            check_param(block_index_info, table_id, column_group_id)) )
      {
        TBSYS_LOG(ERROR, "check_param error, table_id=%ld, column_group_id=%ld",
            table_id, column_group_id);
      }
      else if ( OB_SUCCESS != (ret =
          load_block_index(block_index_info, block_index, table_id, handle)) )
      {
        TBSYS_LOG(ERROR, "load block index error, ret=%d, table_id=%ld"
            "column_group_id=%ld, mode=%d.", 
            ret, table_id, column_group_id, search_mode);
      }
      else
      {
        revert_handle = true;
        ret = block_index.search_one_block_by_key(
            table_id, column_group_id, key, search_mode, pos_info);
      }

      if (revert_handle && OB_SUCCESS != kv_cache_.revert(handle))
      {
        //must revert the handle
        TBSYS_LOG(WARN, "failed to revert  block index cache handle");
      }

      return ret;
    }
  
    int ObBlockIndexCache::next_offset(
        const ObBlockIndexPositionInfo& block_index_info,
        const uint64_t table_id,
        const uint64_t column_group_id,
        const int64_t cur_offset,
        const SearchMode search_mode, 
        ObBlockPositionInfos &pos_info)
    {
      int ret = OB_SUCCESS;
      bool revert_handle = false;
      ObSSTableBlockIndexV2 block_index;
      Handle handle;
    
      if ( OB_SUCCESS != (ret = 
            check_param(block_index_info, table_id, column_group_id)) )
      {
        TBSYS_LOG(ERROR, "check_param error, table_id=%ld, column_group_id=%ld",
            table_id, column_group_id);
      }
      else if ( OB_SUCCESS != (ret =
          load_block_index(block_index_info, block_index, table_id, handle)) )
      {
        TBSYS_LOG(ERROR, "load block index error, ret=%d, table_id=%ld"
            "column_group_id=%ld, cur_offset=%ld, mode=%d.", 
            ret, table_id, column_group_id, cur_offset, search_mode);
      }
      else
      {
        revert_handle = true;
        ret = block_index.search_batch_blocks_by_offset(
            table_id, column_group_id, cur_offset, search_mode, pos_info);
      }

      if (revert_handle && OB_SUCCESS != kv_cache_.revert(handle))
      {
        //must revert the handle
        TBSYS_LOG(WARN, "failed to revert  block index cache handle");
      }
      return ret;
    }

    int ObBlockIndexCache::clear()
    {
      return kv_cache_.clear();
    }
    
    int ObBlockIndexCache::destroy()
    {
      int ret = OB_SUCCESS;

      ret = kv_cache_.destroy();
      if (OB_SUCCESS == ret)
      {
        inited_ = false;
        fileinfo_cache_ = NULL;
      }

      return ret;
    }
  } //end namespace sstable
} //end namespace oceanbase
