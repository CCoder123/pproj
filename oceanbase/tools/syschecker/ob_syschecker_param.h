/**
 * (C) 2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * ob_syschecker_param.h for parse parameter. 
 *
 * Authors:
 *   huating <huating.zmq@taobao.com>
 *
 */
#ifndef OCEANBASE_SYSCHECKER_PARAM_H_
#define OCEANBASE_SYSCHECKER_PARAM_H_

#include <stdint.h>
#include "common/ob_define.h"
#include "common/ob_server.h"

namespace oceanbase 
{ 
  namespace syschecker 
  {

    class ObSyscheckerParam
    {
    public:
      static const int64_t OB_MAX_IP_SIZE               = 64;
      static const int64_t OB_MAX_MERGE_SERVER_STR_SIZE = 2048;
      static const int64_t DEFAULT_TIMEOUT              = 2000000; //2s
      static const int64_t DEFAULT_WRITE_THREAD_COUNT   = 2;
      static const int64_t DEFAULT_READ_THERAD_COUNT    = 20;
      static const int64_t DEFAULT_SYSCHECKER_COUNT     = 1;
      static const int64_t DEFAULT_STAT_DUMP_INTERVAL   = 0;

      ObSyscheckerParam();
      ~ObSyscheckerParam();
    
    public:
      int load_from_config();
      void dump_param();

      inline const common::ObServer& get_root_server() const 
      {
        return root_server_; 
      }

      inline const common::ObServer& get_update_server() const 
      {
        return update_server_; 
      }

      inline const int64_t get_merge_server_count()
      {
        return merge_server_count_;
      }

      inline const common::ObServer* get_merge_server() const 
      {
        return merge_server_; 
      }

      inline const int64_t get_network_time_out() const 
      {
        return network_time_out_; 
      }

      inline const int64_t get_write_thread_count() const 
      {
        return write_thread_count_; 
      }

      inline const int64_t get_read_thread_count() const 
      {
        return read_thread_count_; 
      }

      inline const int64_t get_syschecker_count() const
      {
        return syschecker_count_;
      }

      inline const int64_t get_syschecker_no() const
      {
        return syschecker_no_;
      }

      inline const bool is_specified_read_param() const
      {
        return (specified_read_param_ == 0 ? false : true);
      }

      inline const bool is_operate_full_row() const
      {
        return (operate_full_row_ == 0 ? false : true);
      }

      inline const int64_t get_stat_dump_interval() const 
      {
        return stat_dump_interval_; 
      }      

    private:
      int load_string(char* dest, const int64_t size, 
                      const char* section, const char* name, bool not_null);
      int parse_merge_server(char* str);
      int load_merge_server();

    private:
      common::ObServer root_server_;
      common::ObServer update_server_;
      int64_t merge_server_count_;
      common::ObServer* merge_server_;
      int64_t network_time_out_;
      int64_t write_thread_count_;
      int64_t read_thread_count_;
      int64_t syschecker_count_;
      int64_t syschecker_no_;
      int64_t specified_read_param_;
      int64_t operate_full_row_;
      int64_t stat_dump_interval_;
    };
  } // end namespace syschecker
} // end namespace oceanbase

#endif //OCEANBASE_SYSCHECKER_PARAM_H_
