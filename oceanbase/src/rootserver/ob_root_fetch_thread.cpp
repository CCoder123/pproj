/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_root_fetch_thread.cpp for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */
#include "ob_root_fetch_thread.h"

namespace oceanbase
{
  namespace rootserver
  {
    const int64_t ObRootFetchThread::WAIT_INTERVAL = 100 * 1000; // 100 ms

    ObRootFetchThread::ObRootFetchThread() : recover_ret_(common::OB_SUCCESS), is_recover_done_(false), log_manager_(NULL)
    {
    }

    int ObRootFetchThread::wait_recover_done()
    {
      int count = 0;
      while (!is_recover_done_)
      {
        count++;
        if ((count % 100) == 0)
        {
          TBSYS_LOG(INFO, "wait recover from check point used %d seconds", (count/10));
        }
        usleep(WAIT_INTERVAL);
      }

      return recover_ret_;
    }

    int ObRootFetchThread::got_ckpt(uint64_t ckpt_id)
    {

      TBSYS_LOG(DEBUG, "fetch got checkpoint %d", ckpt_id);

      if (log_manager_ == NULL)
      {
        recover_ret_ = common::OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "log manager is not set");
      }
      else
      {
        recover_ret_ = log_manager_->recover_checkpoint(ckpt_id);
      }

      is_recover_done_ = true;

      TBSYS_LOG(INFO, "recover from checkpoint %d return %d", ckpt_id, recover_ret_);

      return recover_ret_;
    }

    void ObRootFetchThread::set_log_manager(ObRootLogManager* log_manager)
    {
      log_manager_ = log_manager;
    }

  } /* rootserver */
} /* oceanbase */

