/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_join_cache.cpp for ...
 *
 * Authors:
 *   huating <huating.zmq@taobao.com>
 *
 */
#include <tblog.h>
#include "ob_join_cache.h"

namespace oceanbase
{
  namespace mergeserver
  {
    using namespace oceanbase::common;

    ObJoinCache::ObJoinCache() : inited_(false)
    {

    }

    ObJoinCache::~ObJoinCache()
    {

    }

    int ObJoinCache::init(const int64_t max_mem_size)
    {
      int ret = OB_SUCCESS;

      if (inited_)
      {
        //TBSYS_LOG(INFO, "have inited");
        //ret = OB_ERROR;
      }
      else if (OB_SUCCESS != kv_cache_.init(max_mem_size))
      {
        TBSYS_LOG(WARN, "init kv cache fail");
        ret = OB_ERROR;
      }
      else
      {
        inited_ = true;
        TBSYS_LOG_US(DEBUG, "init join cache succ, cache_mem_size=%ld,",
                     max_mem_size);
      }

      return ret;
    }


    int ObJoinCache::clear()
    {
      return kv_cache_.clear();
    }

    int ObJoinCache::destroy()
    {
      inited_ = false;
      return kv_cache_.destroy();
    }

    int ObJoinCache::get_row(const ObJoinCacheKey& key, ObRowCellVec& row_cells)
    {
      int ret     = OB_SUCCESS;
      int64_t pos = 0;
      ObJoinCacheValue row_val;
      Handle handle;

      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
        ret = OB_NOT_INIT;
      }
      else if (NULL == key.row_key_ || key.row_key_size_ <= 0)
      {
        TBSYS_LOG(WARN, "invalid join cache key, ptr=%p, key_len=%d", 
                  key.row_key_, key.row_key_size_);
        ret = OB_INVALID_ARGUMENT;
      }
      else
      {
        ret = kv_cache_.get(key, row_val, handle);
        if (OB_SUCCESS == ret)
        {
          ret = row_cells.deserialize(row_val.buf_, row_val.size_, pos);
          kv_cache_.revert(handle);
        }
      }

      return ret;
    }

    int ObJoinCache::put_row(const ObJoinCacheKey& key, const ObRowCellVec& row_cells)
    {
      int ret = OB_SUCCESS;
      ObJoinCacheValue row_val;

      if (!inited_)
      {
        TBSYS_LOG(WARN, "have not inited");
        ret = OB_NOT_INIT;
      }
      else if (NULL == key.row_key_ || key.row_key_size_ <= 0)
      {
        TBSYS_LOG(WARN, "invalid join cache key, ptr=%p, key_len=%d", 
                  key.row_key_, key.row_key_size_);
        ret = OB_INVALID_ARGUMENT;
      }
      else
      {
        row_val.size_ = row_cells.get_serialize_size();
        row_val.row_cell_vec_ = &row_cells;

        /**
         * put the ObJoinCacheValue instance into join cache, maybe the 
         * row cells are existent in join cache, so ignore the return 
         * value. 
         */
        kv_cache_.put(key, row_val, false);
      }

      return ret;
    }

  } //end namespace mergeserver
} //end namespace oceanbase


