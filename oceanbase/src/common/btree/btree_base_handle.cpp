/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ./btree_base_handle.cpp for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */
#include <stdio.h>
#include "btree_base_handle.h"
#include "btree_root_pointer.h"

namespace oceanbase
{
  namespace common
  {
    /**
     * 构造
     */
    BtreeBaseHandle::BtreeBaseHandle()
    {
      root_pointer_ = NULL;
      ref_count_ = NULL;
    }

    /**
     * 析构
     */
    BtreeBaseHandle::~BtreeBaseHandle()
    {
      clear();
    }

    /**
     * 清理,把引用计数减1
     */
    void BtreeBaseHandle::clear()
    {
      if (ref_count_ != NULL)
      {
        int64_t old_value = 0;
        do
        {
          old_value = *ref_count_;
        }
        while(!BtreeRootPointer::refcas(ref_count_, old_value, old_value - 1));
        ref_count_ = NULL;
      }
    }

    /**
     * root_pointer is null
     */
    bool BtreeBaseHandle::is_null_pointer()
    {
      return (NULL == root_pointer_);
    }

    /**
     * 构造
     */
    BtreeCallback::~BtreeCallback()
    {
    }
  } // end namespace common
} // end namespace oceanbase

