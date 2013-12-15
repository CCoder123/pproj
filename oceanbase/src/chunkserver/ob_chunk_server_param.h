/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * Version: 5567
 *
 * ob_chunk_server_param.h
 *
 * Authors:
 *     maoqi <maoqi@taobao.com>
 * Changes: 
 *     qushan <qushan@taobao.com>
 *     huating <huating.zmq@taobao.com>
 *
 */
#ifndef OCEANBASE_CHUNKSERVER_PARAM_H_
#define OCEANBASE_CHUNKSERVER_PARAM_H_

#include <stdint.h>
#include "common/ob_define.h"
#include "common/ob_server.h"
#include "sstable/ob_block_index_cache.h"
#include "sstable/ob_blockcache.h"

namespace oceanbase 
{ 
  namespace chunkserver 
  {

    class ObChunkServerParam
    {
      public:
        static const int32_t OB_MAX_IP_SIZE = 64;

        ObChunkServerParam();
        ~ObChunkServerParam();
      
      public:
        int load_from_config();
        int reload_from_config(const char* config_file_name);

        inline const char* get_datadir_path() const { return datadir_path_; }
        inline const char* get_application_name() const { return application_name_; }
        inline const char* get_dev_name() const { return dev_name_; }
        inline int32_t get_chunk_server_port() const { return chunk_server_port_; }
        inline int32_t get_task_queue_size() const { return task_queue_size_; }
        inline int32_t get_task_thread_count() const { return task_thread_count_; }
        inline int32_t get_max_migrate_task_count() const { return max_migrate_task_count_; }
        inline int32_t get_retry_times() const {return retry_times_;}
        inline int32_t get_reserve_sstable_copy() const {return reserve_sstable_copy_;}
        inline int64_t get_heartbeat_time_out() const { return heartbeat_time_out_; }
        inline int64_t get_network_time_out() const { return network_time_out_; }
        inline int64_t get_lease_check_interval() const { return lease_check_interval_; }
        inline int64_t get_max_tablets_num() const { return max_tablets_num_; }
        inline int64_t get_max_sstable_size() const { return max_sstable_size_; }
        inline int64_t get_rsync_band_limit() const { return rsync_band_limit_; }
        inline int64_t get_merge_mem_limit() const { return merge_mem_limit_; }
        inline int64_t get_merge_thread_per_disk() const {return merge_thread_per_disk_;}
        inline int64_t get_max_merge_thread() const {return max_merge_thread_num_;}
        inline int64_t get_merge_threshold_load_high() const { return merge_threshold_load_high_;}
        inline int64_t get_merge_threshold_request_high() const {return merge_threshold_request_high_;}
        inline int64_t get_merge_delay_interval() const {return merge_delay_interval_;}
        inline int64_t get_merge_delay_for_lsync() const {return merge_delay_for_lsync_;}
        inline int64_t get_merge_scan_use_preread() const {return merge_scan_use_preread_;}
        inline int64_t get_merge_timeout() const {return merge_timeout_;}
        inline int64_t get_merge_pause_row_count() const { return merge_pause_row_count_; }
        inline int64_t get_merge_pause_sleep_time() const { return merge_pause_sleep_time_; }
        inline int64_t get_merge_highload_sleep_time() const { return merge_highload_sleep_time_; }
        
        inline int64_t get_merge_adjust_ratio() const {return merge_adjust_ratio_;}
        inline int64_t get_max_version_gap() const {return max_version_gap_;}
        inline int64_t get_min_merge_interval() const {return min_merge_interval_;}
        inline int64_t get_min_drop_cache_wait_time() const {return min_drop_cache_wait_time_;}
        inline int64_t get_fetch_ups_interval() const { return fetch_ups_interval_; }
        inline int64_t get_task_left_time() const { return task_left_time_; }
        inline int64_t get_write_sstable_io_type() const {return write_sstable_io_type_; }
        
        inline const sstable::ObBlockCacheConf& get_block_cache_conf() const { return bc_conf_; }
        inline const sstable::ObBlockIndexCacheConf& get_block_index_cache_conf() const { return bic_conf_; }
        inline const sstable::ObBlockIndexCacheConf& get_join_cache_conf() const { return jc_conf_; }

        inline const common::ObServer& get_root_server() const { return root_server_; }

      private:
        int load_string(char* dest, const int32_t size, 
            const char* section, const char* name, bool not_null);
      private:
        char datadir_path_[common::OB_MAX_FILE_NAME_LENGTH];
        char application_name_[common::OB_MAX_APP_NAME_LENGTH];

        //char root_server_ip_[OB_MAX_IP_SIZE];
        //int32_t root_server_port_;
        char dev_name_[OB_MAX_IP_SIZE];
        int32_t chunk_server_port_;
        int32_t retry_times_;
        common::ObServer root_server_;


        int32_t task_queue_size_;
        int32_t task_thread_count_;
        int32_t max_migrate_task_count_;
        int32_t reserve_sstable_copy_;

        int64_t heartbeat_time_out_;
        int64_t network_time_out_;
        int64_t lease_check_interval_;

        int64_t max_tablets_num_;
        int64_t max_sstable_size_;

        int64_t rsync_band_limit_;
        int64_t merge_mem_limit_;
        int64_t merge_thread_per_disk_;
        int64_t max_merge_thread_num_;
        int64_t merge_threshold_load_high_;
        int64_t merge_threshold_request_high_;
        int64_t merge_delay_interval_;
        int64_t merge_delay_for_lsync_;
        int64_t merge_scan_use_preread_;
        int64_t merge_timeout_;
        int64_t merge_pause_row_count_;
        int64_t merge_pause_sleep_time_;
        int64_t merge_highload_sleep_time_;
        int64_t merge_adjust_ratio_;
        int64_t max_version_gap_;
        int64_t min_merge_interval_;
        int64_t min_drop_cache_wait_time_;
        int64_t fetch_ups_interval_;
        int64_t task_left_time_;
        int64_t write_sstable_io_type_;

        sstable::ObBlockCacheConf bc_conf_;
        sstable::ObBlockIndexCacheConf bic_conf_;
        sstable::ObBlockIndexCacheConf jc_conf_;

    };

  } // end namespace chunkserver
} // end namespace oceanbase


#endif //OCEANBASE_CHUNKSERVER_PARAM_H_

