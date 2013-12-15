/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_table_mgr.h for ...
 *
 * Authors:
 *   yubai <yubai.lk@taobao.com>
 *
 */
#ifndef  OCEANBASE_UPDATESERVER_TABLE_MGR_H_
#define  OCEANBASE_UPDATESERVER_TABLE_MGR_H_
#include <sys/types.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <new>
#include <algorithm>
#include "ob_atomic.h"
#include "common/ob_define.h"
#include "common/ob_vector.h"
#include "common/page_arena.h"
#include "common/hash/ob_hashmap.h"
#include "common/ob_list.h"
#include "common/ob_regex.h"
#include "common/ob_fileinfo_manager.h"
#include "common/ob_tsi_factory.h"
#include "common/ob_spin_rwlock.h"
#include "common/ob_timer.h"
#include "sstable/ob_sstable_row.h"
#include "sstable/ob_sstable_block_builder.h"
#include "sstable/ob_sstable_trailer.h"
#include "sstable/ob_sstable_reader.h"
#include "sstable/ob_seq_sstable_scanner.h"
#include "sstable/ob_sstable_getter.h"
#include "ob_ups_utils.h"
#include "ob_sstable_mgr.h"
#include "ob_column_filter.h"
#include "ob_schema_mgr.h"
#include "ob_memtable.h"
#include "ob_memtable_rowiter.h"

namespace oceanbase
{
  namespace updateserver
  {
    typedef MemTableTransHandle TableTransHandle;

    class ITableIterator : public common::ObIterator
    {
      public:
        virtual ~ITableIterator() {};
      public:
        virtual bool is_multi_update() = 0;
    };

    class MemTableEntityIterator : public ITableIterator
    {
      public:
        MemTableEntityIterator();
        ~MemTableEntityIterator();
      public:
        virtual int next_cell();
        virtual int get_cell(common::ObCellInfo **cell_info);
        virtual int get_cell(common::ObCellInfo **cell_info, bool *is_row_changed);
        virtual bool is_multi_update()
        {
          return is_multi_update_;
        };
        void set_multi_update(const bool is_multi_update)
        {
          is_multi_update_ = is_multi_update;
        };
        void reset();
        MemTableIterator &get_memtable_iter();
      private:
        MemTableIterator memtable_iter_;
        bool is_multi_update_;
    };

    class SSTableEntityIterator : public ITableIterator
    {
      public:
        SSTableEntityIterator();
        ~SSTableEntityIterator();
      public:
        virtual int next_cell();
        virtual int get_cell(common::ObCellInfo **cell_info);
        virtual int get_cell(common::ObCellInfo **cell_info, bool *is_row_changed);
        virtual bool is_multi_update()
        {
          return false;
        };
        void reset();
        inline common::ObGetParam &get_get_param();
        void set_sstable_iter(common::ObIterator *sstable_iter);
        void set_column_filter(ColumnFilter *column_filter);
        sstable::ObSSTableGetter &get_sstable_getter();
        sstable::ObSeqSSTableScanner &get_sstable_scanner();
      private:
        common::ObGetParam get_param_;
        common::ObIterator *sstable_iter_;
        sstable::ObSSTableGetter sst_getter_;
        sstable::ObSeqSSTableScanner sst_scanner_;
        ColumnFilter *column_filter_;
        //common::ObCellInfo cell_info_;
        //common::ObStringBuf string_buf_;
        common::ObCellInfo *cur_ci_ptr_;
        //bool is_iter_end_;
        bool is_row_changed_;
        bool row_has_changed_;
        //bool row_has_expected_column_;
        //bool row_has_returned_column_;
        //bool need_not_next_;
        //bool is_sstable_iter_end_;
    };

    class ITableUtils
    {
      public:
        virtual ~ITableUtils() {};
      public:
        virtual ITableIterator &get_table_iter() = 0;
        virtual TableTransHandle &get_trans_handle() = 0;
        virtual void reset() = 0;
    };

    class MemTableUtils : public ITableUtils
    {
      public:
        MemTableUtils();
        ~MemTableUtils();
      public:
        virtual ITableIterator &get_table_iter();
        virtual TableTransHandle &get_trans_handle();
        virtual void reset();
      private:
        MemTableEntityIterator table_iter_;
        TableTransHandle trans_handle_;
    };

    class SSTableUtils : public ITableUtils
    {
      public:
        SSTableUtils();
        ~SSTableUtils();
      public:
        virtual ITableIterator &get_table_iter();
        virtual TableTransHandle &get_trans_handle();
        virtual void reset();
      private:
        SSTableEntityIterator table_iter_;
        TableTransHandle trans_handle_;
    };

    class TableItem;
    class ITableEntity
    {
      public:
        struct SSTableMeta
        {
          int64_t time_stamp;
        };
      public:
        ITableEntity(TableItem &table_item) : table_item_(table_item)
        {
        };
      public:
        virtual ~ITableEntity() {};
      public:
        virtual ITableUtils *get_tsi_tableutils(const int64_t index) = 0;
        virtual int get(TableTransHandle &trans_handle,
                        const uint64_t table_id,
                        const common::ObString &row_key,
                        ColumnFilter *column_filter,
                        ITableIterator *iter) = 0;
        virtual int scan(TableTransHandle &trans_handle,
                        const common::ObScanParam &scan_param,
                        ITableIterator *iter) = 0;
        virtual int start_transaction(TableTransHandle &trans_handle) = 0;
        virtual int end_transaction(TableTransHandle &trans_handle) = 0;
        virtual void ref() = 0;
        virtual void deref() = 0;
      public:
        inline TableItem &get_table_item()
        {
          return table_item_;
        };
        inline static ColumnFilter *get_tsi_columnfilter()
        {
          using namespace common;
          return GET_TSI(ColumnFilter);
        };
      public:
        static ColumnFilter *build_columnfilter(const common::ObScanParam &scan_param, ColumnFilter *column_filter);
      protected:
        static const int64_t MAX_TABLE_UTILS_NUM = 128;
        TableItem &table_item_;
    };

    class MemTableEntity : public ITableEntity
    {
      struct TableUtilsSet
      {
        MemTableUtils data[MAX_TABLE_UTILS_NUM];
      };
      public:
        MemTableEntity(TableItem &table_item);
        virtual ~MemTableEntity();
      public:
        virtual ITableUtils *get_tsi_tableutils(const int64_t index);
        virtual int get(TableTransHandle &trans_handle,
                        const uint64_t table_id,
                        const common::ObString &row_key,
                        ColumnFilter *column_filter,
                        ITableIterator *iter);
        virtual int scan(TableTransHandle &trans_handle,
                        const common::ObScanParam &scan_param,
                        ITableIterator *iter);
        virtual int start_transaction(TableTransHandle &trans_handle);
        virtual int end_transaction(TableTransHandle &trans_handle);
        virtual void ref();
        virtual void deref();
        MemTable &get_memtable();
      private:
        MemTable memtable_;
    };

    class SSTableEntity : public ITableEntity
    {
      struct TableUtilsSet
      {
        SSTableUtils data[MAX_TABLE_UTILS_NUM];
      };
      public:
        SSTableEntity(TableItem &table_item);
        virtual ~SSTableEntity();
      public:
        virtual ITableUtils *get_tsi_tableutils(const int64_t index);
        virtual int get(TableTransHandle &trans_handle,
                        const uint64_t table_id,
                        const common::ObString &row_key,
                        ColumnFilter *column_filter,
                        ITableIterator *iter);
        virtual int scan(TableTransHandle &trans_handle,
                        const common::ObScanParam &scan_param,
                        ITableIterator *iter);
        virtual int start_transaction(TableTransHandle &trans_handle);
        virtual int end_transaction(TableTransHandle &trans_handle);
        virtual void ref();
        virtual void deref();
        uint64_t get_sstable_id() const;
        void set_sstable_id(const uint64_t sstable_id);
        int init_sstable_meta(SSTableMeta &sst_meta);
        void destroy_sstable_meta();
      private:
        uint64_t sstable_id_;
        common::ModulePageAllocator mod_;
        common::ModuleArena allocator_;
        sstable::ObSSTableReader *sstable_reader_;
    };

    class TableItem
    {
      public:
        enum Stat
        {
          UNKNOW = 0,
          ACTIVE = 1,   // 是活跃表 可作为起始状态
          FREEZING = 2, // 正在冻结
          FROZEN = 3,   // 已经冻结
          DUMPING = 4,  // 正在转储
          DUMPED = 5,   // 已经转储sstable
          DROPING = 6,  // 正在释放
          DROPED = 7,   // 内存表已释放 可作为起始状态
        };
      public:
        TableItem();
        ~TableItem();
      public:
        ITableEntity *get_table_entity(int64_t &sstable_percent);
        MemTable &get_memtable();
        int init_sstable_meta();
        Stat get_stat() const;
        void set_stat(const Stat stat);
        uint64_t get_sstable_id() const;
        void set_sstable_id(const uint64_t sstable_id);
        int64_t get_time_stamp() const;
        int64_t get_sstable_loaded_time() const;
        int do_freeze(const uint64_t clog_id, const int64_t time_stamp);
        int do_dump();
        int do_drop();
        int freeze_memtable();
        bool dump_memtable();
        bool drop_memtable();
        bool erase_sstable();
      public:
        int64_t inc_ref_cnt();
        int64_t dec_ref_cnt();
      private:
        MemTableEntity memtable_entity_;
        SSTableEntity sstable_entity_;
        Stat stat_;
        volatile int64_t ref_cnt_;
        uint64_t clog_id_;
        MemTableRowIterator row_iter_;
        int64_t time_stamp_;
        int64_t sstable_loaded_time_;
    };

    typedef common::ObList<ITableEntity*> TableList;

    class TableMgr : public ISSTableObserver, public IExternMemTotal
    {
      struct TableItemKey
      {
        SSTableID sst_id;
        uint64_t reserver;
        TableItemKey()
        {
        };
        TableItemKey(const SSTableID &other_sst_id)
        {
          sst_id = other_sst_id;
        };
        inline int operator- (const TableItemKey &other) const
        {
          return SSTableID::compare(sst_id, other.sst_id);
        };
      };
      typedef common::KeyBtree<TableItemKey, TableItem*> TableItemMap;
      typedef common::hash::SimpleAllocer<TableItem> TableItemAllocator;
      public:
        enum FreezeType
        {
          AUTO_TRIG = 0,
          FORCE_MINOR = 1,
          FORCE_MAJOR = 2,
        };
      public:
        TableMgr();
        ~TableMgr();
      public:
        virtual int add_sstable(const uint64_t sstable_id);
        virtual int erase_sstable(const uint64_t sstable_id);
      public:
        int init();
        void destroy();

        // 在播放日志完成后调用
        int sstable_scan_finished(const int64_t minor_num_limit);

        // 根据版本号范围获取一组table entity
        int acquire_table(const common::ObVersionRange &version_range,
                          uint64_t &max_version,
                          TableList &table_list);
        // 释放一组table entity
        void revert_table(const TableList &table_list);

        // 获取当前的活跃内存表
        TableItem *get_active_memtable();
        // 归还活跃表
        void revert_active_memtable(TableItem *table_item);

        // 只有slave或回放日志时调用
        // 冻结当前活跃表 frozen version为传入参数
        int replay_freeze_memtable(const uint64_t new_version,
                                  const uint64_t frozen_version,
                                  const uint64_t clog_id,
                                  const int64_t time_stamp = 0);
        // 只有master调用
        // 主线程定期调用 内存占用超过mem_limit情况下冻结活跃表
        // 当前版本冻结表个数超过num_limit情况下执行major free 由定时线程触发异步任务
        // 执行major free的时候要与slave同步日志
        int try_freeze_memtable(const int64_t mem_limit, const int64_t num_limit,
                                const int64_t min_major_freeze_interval,
                                uint64_t &new_version, uint64_t &frozen_version,
                                uint64_t &clog_id, int64_t &time_stamp,
                                bool &major_version_changed);
        int try_freeze_memtable(const FreezeType freeze_type,
                                uint64_t &new_version, uint64_t &frozen_version,
                                uint64_t &clog_id, int64_t &time_stamp,
                                bool &major_version_changed);
        // 工作线程调用 将冻结表转储为sstable
        // 返回false表示没有冻结表需要转储了
        bool try_dump_memtable();
        // 主线程定期调用 内存占用超过mem_limit情况下释放冻结表 由定时线程触发异步任务
        bool try_drop_memtable(const int64_t mem_limit);
        bool try_drop_memtable(const bool force);
        // 工作线程调用 卸载并删除生成时间到现在超过time_limit的sstable文件
        void try_erase_sstable(const int64_t time_limit);
        void try_erase_sstable(const bool force);

        int check_sstable_id();

        void log_table_info();

        void set_memtable_attr(const MemTableAttr &memtable_attr);
        int get_memtable_attr(MemTableAttr &memtable_attr);

        int64_t get_frozen_memused() const;
        int64_t get_frozen_memtotal() const;
        int64_t get_frozen_rowcount() const;
        uint64_t get_active_version();
        int get_table_time_stamp(const uint64_t major_version, int64_t &time_stamp);

        void dump_memtable2text(const common::ObString &dump_dir);
        int clear_active_memtable();

        virtual int64_t get_extern_mem_total();

        void set_warm_up_percent(const int64_t warm_up_percent);
      private:
        TableItem *freeze_active_(const uint64_t new_version);
        void revert_memtable_(TableItem *table_item);
        int64_t get_warm_up_percent_();
      private:
        bool inited_;
        bool sstable_scan_finished_;
        TableItemAllocator table_allocator_;
        common::SpinRWLock map_lock_;
        TableItemMap table_map_;

        uint64_t cur_major_version_;
        uint64_t cur_minor_version_;
        TableItem *active_table_item_;

        int64_t frozen_memused_;
        int64_t frozen_memtotal_;
        int64_t frozen_rowcount_;
        int64_t last_major_freeze_time_;

        MemTableAttr memtable_attr_;

        int64_t cur_warm_up_percent_;
    };

    extern void thread_read_prepare();
    extern void thread_read_complete();

    class MajorFreezeDuty : public common::ObTimerTask
    {
      public:
        static const int64_t SCHEDULE_PERIOD = 1000000L;
      public:
        MajorFreezeDuty() {};
        virtual ~MajorFreezeDuty() {};
        virtual void runTimerTask();
    };

    class HandleFrozenDuty : public common::ObTimerTask
    {
      public:
        static const int64_t SCHEDULE_PERIOD = 10L * 60L * 1000000L;
      public:
        HandleFrozenDuty() {};
        virtual ~HandleFrozenDuty() {};
        virtual void runTimerTask();
    };
  }
}

#endif //OCEANBASE_UPDATESERVER_TABLE_MGR_H_



