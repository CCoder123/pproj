/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * Version: 5567
 *
 * ob_chunk_service.h
 *
 * Authors:
 *     qushan <qushan@taobao.com>
 * Changes: 
 *     maoqi <maoqi@taobao.com>
 *     huating <huating.zmq@taobao.com>
 *     fangji <fangji.hcm@taobao.com>
 *
 */
#ifndef OCEANBASE_CHUNKSERVER_CHUNKSERVICE_H_
#define OCEANBASE_CHUNKSERVER_CHUNKSERVICE_H_

#include "tbnet.h"
#include "common/ob_define.h"
#include "common/thread_buffer.h"
#include "common/ob_timer.h"

namespace oceanbase 
{ 
  namespace common
  {
    class ObDataBuffer;
    class ObSchemaManager;
    class ObSchemaManagerV2;
  }
  namespace chunkserver 
  {
    class ObChunkServer;

    class ObChunkService
    {
      public:
        ObChunkService();
        ~ObChunkService();
      public:
        int initialize(ObChunkServer* chunk_server);
        int start();
        int destroy();
      public:
        int do_request(
            const int32_t packet_code,
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection,
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);
      private:
        int cs_get(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_batch_get(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);


        int cs_scan(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_drop_old_tablets(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_heart_beat(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_create_tablet(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_load_tablet(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);


        int cs_migrate_tablet(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_get_migrate_dest_loc(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_dump_tablet_image(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);
        
        int cs_fetch_stats(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);
        
        int cs_start_gc(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

        int cs_reload_conf(
            const int32_t version,
            const int32_t channel_id,
            tbnet::Connection* connection, 
            common::ObDataBuffer& in_buffer, 
            common::ObDataBuffer& out_buffer);

      private:
        class LeaseChecker : public common::ObTimerTask 
        {
          public:
            LeaseChecker(ObChunkService* service) : service_(service) {}
            ~LeaseChecker() {}
          public:
            virtual void runTimerTask();
          private:
            ObChunkService* service_;
        };
        
        class StatUpdater : public common::ObTimerTask
        {
          public:
            StatUpdater () : pre_request_count_(0),current_request_count_(0) {}
            ~StatUpdater() {}
          public:
            virtual void runTimerTask();
          private:
            int64_t pre_request_count_;
            int64_t current_request_count_;
        };

        class MergeTask : public common::ObTimerTask
        {
          public:
            MergeTask (ObChunkService* service) 
              : frozen_version_(0), task_scheduled_(false), service_(service)  {}
          public:
            inline int64_t get_last_frozen_version() const { return frozen_version_; }
            void set_frozen_version(int64_t version) { frozen_version_ = version; }
            inline bool is_scheduled() const { return task_scheduled_; }
            inline void set_scheduled() { task_scheduled_ = true; }
            inline void unset_scheduled() { task_scheduled_ = false; }
            virtual void runTimerTask();
          private:
            int64_t frozen_version_;
            bool task_scheduled_;
            ObChunkService* service_;
        };

        class FetchUpsTask : public common::ObTimerTask
        {
          public:
            FetchUpsTask (ObChunkService* service) : service_(service) {}
          public:
            virtual void runTimerTask();
          private:
            ObChunkService* service_;
        };

      private:
        int load_tablets();
        int register_self();
        int register_self_busy_wait(int32_t &status);
        int report_tablets_busy_wait();
        int fetch_schema_busy_wait(common::ObSchemaManagerV2 *schema);
        bool is_valid_lease();
        void get_merge_delay_interval();
      private:
        DISALLOW_COPY_AND_ASSIGN(ObChunkService);
        ObChunkServer* chunk_server_;
        bool inited_;
        bool service_started_;
        bool in_register_process_;
        int64_t service_expired_time_;
        int64_t merge_delay_interval_;
        volatile uint32_t migrate_task_count_;

        common::ObTimer timer_;
        LeaseChecker lease_checker_;
        StatUpdater  stat_updater_;
        MergeTask    merge_task_;
        FetchUpsTask fetch_ups_task_;
    };


  } // end namespace chunkserver
} // end namespace oceanbase


#endif //OCEANBASE_CHUNKSERVER_CHUNKSERVICE_H_

