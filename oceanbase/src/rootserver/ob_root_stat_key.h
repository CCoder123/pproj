/*
 * Copyright (C) 2007-2011 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Description here
 *
 * Version: $Id$
 *
 * Authors:
 *   Zhifeng YANG <zhuweng.yzf@taobao.com>
 *     - some work details here
 */
#ifndef _OB_ROOT_STAT_KEY_H
#define _OB_ROOT_STAT_KEY_H 1

namespace oceanbase
{
namespace rootserver
{
enum ObRootStatKey
{
  OB_RS_STAT_RESERVE = 0,
  OB_RS_STAT_COMMON = 1,
  OB_RS_STAT_START_TIME = 2,
  OB_RS_STAT_PROGRAM_VERSION = 3,
  OB_RS_STAT_PID = 4,
  OB_RS_STAT_LOCAL_TIME = 5,
  OB_RS_STAT_MEM = 6,
  OB_RS_STAT_RS_STATUS = 7,
  OB_RS_STAT_FROZEN_VERSION = 8,
  OB_RS_STAT_SCHEMA_VERSION = 9,
  OB_RS_STAT_LOG_SEQUENCE = 10,
  OB_RS_STAT_LOG_FILE_ID = 11,
  OB_RS_STAT_TABLE_NUM = 12,
  OB_RS_STAT_TABLET_NUM = 13,
  OB_RS_STAT_REPLICAS_NUM = 14,
  OB_RS_STAT_CS_NUM = 15,
  OB_RS_STAT_MS_NUM = 16,
  OB_RS_STAT_CS = 17,
  OB_RS_STAT_MS = 18,
  OB_RS_STAT_UPS = 19,
  OB_RS_STAT_RS_SLAVE = 20,
  OB_RS_STAT_OPS_GET = 21,
  OB_RS_STAT_OPS_SCAN = 22,
  OB_RS_STAT_RS_SLAVE_NUM = 23,
  OB_RS_STAT_FROZEN_TIME = 24,
  OB_RS_STAT_CLIENT_CONF = 25,
  OB_RS_STAT_SSTABLE_DIST = 26,
  OB_RS_STAT_END
};

extern const char* OB_RS_STAT_KEYSTR[];

} // end namespace rootserver
} // end namespace oceanbase

#endif /* _OB_ROOT_STAT_KEY_H */

