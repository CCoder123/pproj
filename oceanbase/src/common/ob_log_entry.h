/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_log_entry.h for ...
 *
 * Authors:
 *   yanran <yanran.hfs@taobao.com>
 *
 */
#ifndef OCEANBASE_COMMON_OB_LOG_ENTRY_H_
#define OCEANBASE_COMMON_OB_LOG_ENTRY_H_

#include "ob_record_header.h"

namespace oceanbase
{
  namespace common
  {
    extern const char* DEFAULT_CKPT_EXTENSION;

    enum LogCommand
    {
      //// Base command ... ////
      OB_LOG_SWITCH_LOG = 101,  //日志切换命令
      OB_LOG_CHECKPOINT = 102,  //checkpoint操作
      OB_LOG_NOP = 103,

      //// UpdateServer ... 200 - 399 ////
      OB_LOG_UPS_MUTATOR = 200,
      OB_UPS_SWITCH_SCHEMA = 201,

      //// RootServer ...  400 - 599 ////
      OB_RT_SCHEMA_SYNC = 400,
      OB_RT_CS_REGIST = 401,
      OB_RT_MS_REGIST = 402,
      OB_RT_SERVER_DOWN = 403,
      OB_RT_CS_LOAD_REPORT = 404,
      OB_RT_CS_MIGRATE_DONE = 405,
      OB_RT_CS_START_SWITCH_ROOT_TABLE = 406,
      OB_RT_START_REPORT = 407,
      OB_RT_REPORT_TABLETS = 408,
      OB_RT_ADD_NEW_TABLET = 409,
      OB_RT_CREATE_TABLE_DONE = 410,
      OB_RT_BEGIN_BALANCE = 411,
      OB_RT_BALANCE_DONE = 412,
      OB_RT_US_MEM_FRZEEING = 413,
      OB_RT_US_MEM_FROZEN = 414,
      OB_RT_CS_START_MERGEING = 415,
      OB_RT_CS_MERGE_OVER = 416,
      OB_RT_CS_UNLOAD_DONE = 417,
      OB_RT_US_UNLOAD_DONE = 418,
      OB_RT_DROP_CURRENT_BUILD = 419,
      OB_RT_DROP_LAST_CS_DURING_MERGE = 420,
      OB_RT_SYNC_FROZEN_VERSION = 421,
      OB_RT_SET_UPS_LIST = 422,
      OB_RT_SET_CLIENT_CONFIG = 423,
      //// ChunkServer ... 600 - 799 ////

      //// Base command ... ////
      OB_LOG_UNKNOWN = 199
    };

    /**
     * @brief 日志项
     * 一条日志项由四部分组成:
     *     ObRecordHeader + 日志序号 + LogCommand + 日志内容
     * ObLogEntry中保存ObRecordHeader, 日志序号, LogCommand 三部分
     * ObLogEntry中的data_checksum_项是对"日志序号", "LogCommand", "日志内容" 部分的校验
     */
    struct ObLogEntry
    {
      ObRecordHeader header_;
      uint64_t seq_;
      int32_t cmd_;

      static const int16_t MAGIC_NUMER = 0xAAAALL;
      static const int16_t LOG_VERSION = 1;

      ObLogEntry()
      {
        memset(this, 0x00, sizeof(ObLogEntry));
      }

      /**
       * @brief 设置日志序号
       */
      void set_log_seq(const uint64_t seq) {seq_ = seq;}

      /**
       * @brief 设置LogCommand
       */
      void set_log_command(const int32_t cmd) {cmd_ = cmd;}

      /**
       * @brief fill all fields of ObRecordHeader
       * 调用该函数需要保证set_log_seq函数已经被调用
       * @param [in] log_data 日志内容缓冲区地址
       * @param [in] data_len 缓冲区长度
       */
      int fill_header(const char* log_data, const int64_t data_len);

      /**
       * @brief 计算日志序号+LogCommand+日志内容的校验和
       * @param [in] buf 日志内容缓冲区地址
       * @param [in] len 缓冲区长度
       */
      int64_t calc_data_checksum(const char* log_data, const int64_t data_len) const;

      /**
       * After deserialization, this function get the length of log
       */
      int32_t get_log_data_len() const
      {
        return header_.data_length_ - sizeof(uint64_t) - sizeof(LogCommand);
      }

      int check_header_integrity() const;

      /**
       * 调用deserialization之后, 调用该函数检查数据正确性
       * @param [in] log 日志内容缓冲区地址
       */
      int check_data_integrity(const char* log_data) const;

      static int get_header_size() {return sizeof(ObRecordHeader) + sizeof(uint64_t) + sizeof(LogCommand);}

      NEED_SERIALIZE_AND_DESERIALIZE;
    };
  } // end namespace common
} // end namespace oceanbase

#endif  //OCEANBASE_COMMON_OB_LOG_ENTRY_H_


