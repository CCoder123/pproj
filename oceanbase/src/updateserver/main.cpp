/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * main.cpp for ...
 *
 * Authors:
 *   rizhao <rizhao.ych@taobao.com>
 *
 */
#include "common/ob_malloc.h"
#include "ob_update_server_main.h"
#include <malloc.h>

using namespace oceanbase::common;
using namespace oceanbase::updateserver;

namespace
{
  const char* PUBLIC_SECTION_NAME = "public";
  static const int DEFAULT_MMAP_MAX_VAL = 65536;
}

int main(int argc, char** argv)
{
  mallopt(M_MMAP_MAX, DEFAULT_MMAP_MAX_VAL);
  ob_init_memory_pool();
  ObUpdateServerMain* ups = ObUpdateServerMain::get_instance();
  int ret = OB_SUCCESS;
  if (NULL == ups) 
  {
    fprintf(stderr, "cannot start updateserver, new ObUpdateServerMain failed\n");
    ret = OB_ERROR;
  }
  else 
  {
    ret = ups->start(argc, argv, PUBLIC_SECTION_NAME);
    if (OB_SUCCESS == ret)
    {
      ups->destroy();
    }
  }

  return ret;
}


