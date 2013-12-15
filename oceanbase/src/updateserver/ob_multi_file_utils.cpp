/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_multi_file_utils.cpp for ...
 *
 * Authors:
 *   yubai <yubai.lk@taobao.com>
 *
 */
#include "ob_multi_file_utils.h"

namespace oceanbase
{
  using namespace common;
  namespace updateserver
  {
    MultiFileUtils::MultiFileUtils()
    {
    }

    MultiFileUtils::~MultiFileUtils()
    {
      close();
    }

    int MultiFileUtils::open(const ObString &fname, const bool dio, const bool is_create, const bool is_trunc, const int64_t align_size)
    {
      int32_t ret = OB_SUCCESS;
      if (NULL == fname.ptr())
      {
        TBSYS_LOG(WARN, "fname null pointer");
        ret = OB_INVALID_ARGUMENT;
      }
      else
      {
        close();
        const char *iter = fname.ptr();
        while (NULL != iter)
        {
          ObString fname;
          fname.assign(const_cast<char*>(iter), strlen(iter));
          ObFileAppender *file = file_alloc_.allocate();
          if (NULL == file)
          {
            TBSYS_LOG(WARN, "allocate file appender fail [%s]", iter);
            ret = OB_ERROR;
            break;
          }
          else if (OB_SUCCESS != (ret = file->open(fname, dio, is_create, is_trunc, align_size)))
          {
            TBSYS_LOG(WARN, "open [%s] fail ret=%d", iter, ret);
            break;
          }
          else if (OB_SUCCESS != (ret = flist_.push_back(file)))
          {
            TBSYS_LOG(WARN, "push fileinfo to list fail [%s] ret=%d", iter, ret);
            file->close();
            break;
          }
          TBSYS_LOG(INFO, "open [%s] pos=[%ld] succ", iter, flist_.size() - 1);
          while ('\0' != *iter)
          {
            iter++;
          }
          iter++;
          if ('\0' == *iter)
          {
            break;
          }
        }
        if (OB_SUCCESS != ret)
        {
          close();
        }
      }
      return ret;
    }

    void MultiFileUtils::close()
    {
      ObList<ObFileAppender*>::iterator iter;
      for (iter = flist_.begin(); iter != flist_.end(); iter++)
      {
        ObFileAppender *file = *iter;
        if (NULL != file)
        {
          file->close();
          file_alloc_.deallocate(file); 
          file = NULL;
        }
      }
      flist_.clear();
    }

    int64_t MultiFileUtils::get_file_pos() const
    {
      int64_t ret = -1;
      ObList<ObFileAppender*>::const_iterator iter;
      ObFileAppender *file = NULL;
      if (0 == flist_.size())
      {
        TBSYS_LOG(WARN, "no file open");
      }
      else if (flist_.end() == (iter = flist_.begin()))
      {
        TBSYS_LOG(WARN, "flist begin equal end");
      }
      else if (NULL == (file = *iter))
      {
        TBSYS_LOG(WARN, "invalid file pos=[0]");
      }
      else
      {
        ret = file->get_file_pos();
      }
      return ret;
    }

    int MultiFileUtils::append(const void *buf, const int64_t count, bool is_fsync)
    {
      int ret = OB_SUCCESS;
      if (0 == flist_.size())
      {
        TBSYS_LOG(WARN, "no file open");
        ret = OB_ERROR;
      }
      else
      {
        ObList<ObFileAppender*>::iterator iter;
        int64_t pos = 0;
        for (iter = flist_.begin(); iter != flist_.end(); iter++, pos++)
        {
          ObFileAppender *file = *iter;
          if (NULL == file)
          {
            TBSYS_LOG(WARN, "invalid file pos=[%ld] buf=%p count=%ld", pos, count);
            ret = OB_ERROR;
            break;
          }
          ret = file->append(buf, count, is_fsync);
          if (OB_SUCCESS != ret)
          {
            TBSYS_LOG(WARN, "append file fail pos=[%ld] ret=%d buf=%d count=%ld", pos, ret, buf, count);
            break;
          }
        }
      }
      return ret;

    }

    int MultiFileUtils::fsync()
    {
      int ret = OB_SUCCESS;
      if (0 == flist_.size())
      {
        TBSYS_LOG(WARN, "no file open");
        ret = OB_ERROR;
      }
      else
      {
        ObList<ObFileAppender*>::iterator iter;
        int64_t pos = 0;
        for (iter = flist_.begin(); iter != flist_.end(); iter++, pos++)
        {
          ObFileAppender *file = *iter;
          if (NULL == file)
          {
            TBSYS_LOG(WARN, "invalid file pos=[%ld]", pos);
            ret = OB_ERROR;
            break;
          }
          ret = file->fsync();
          if (OB_SUCCESS != ret)
          {
            TBSYS_LOG(WARN, "fsync fail pos=[%ld] ret=%d", pos, ret);
            break;
          }
        }
      }
      return ret;
    }
  }
}



