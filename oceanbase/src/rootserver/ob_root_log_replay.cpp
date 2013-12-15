/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_root_log_replay.cpp for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */
#include "ob_root_log_replay.h"

namespace oceanbase
{
  namespace rootserver
  {
    ObRootLogReplay::ObRootLogReplay()
    {
      is_initialized_ = false;
    }

    ObRootLogReplay::~ObRootLogReplay()
    {
    }

    void ObRootLogReplay::set_log_manager(ObRootLogManager* log_manager)
    {
      log_manager_ = log_manager;
    }

    int ObRootLogReplay::replay(common::LogCommand cmd, uint64_t seq, const char* log_data, const int64_t data_len)
    {
      UNUSED(seq);
      int ret = log_manager_->get_log_worker()->apply(cmd, log_data, data_len);
      if (ret != common::OB_SUCCESS)
      {
        TBSYS_LOG(ERROR, "fatal error, replay log failed, err=[%d]. Quit...", ret);
        exit(120);
        // log_manager_->get_log_worker()->exit();
      }
      return ret;
    }
  } /* rootserver */
} /* oceanbase */

