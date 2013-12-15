/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * Version: 5567
 *
 * ob_file_recycle.h
 *
 * Authors:
 *     maoqi <maoqi@taobao.com>
 * Changes: 
 *     qushan <qushan@taobao.com>
 *     huating <huating.zmq@taobao.com>
 *
 */
#ifndef OCEANBASE_CHUNKSERVER_OB_FILE_RECYCLE_H_
#define OCEANBASE_CHUNKSERVER_OB_FILE_RECYCLE_H_
#include <dirent.h>
#include "common/ob_define.h"
#include "ob_tablet.h"
#include "chunkserver/ob_tablet_image.h"


namespace oceanbase 
{
  namespace chunkserver
  {
    class ObTabletManager;

    /**
     * recycle expired sstable files regularly. 
     * chunkserver keeps two version tablets in memory,
     * recycle every expired sstable which upgrade to 
     * new version.
     */
    class ObRegularRecycler
    {
      public:
        ObRegularRecycler(ObTabletManager& manager);
        ~ObRegularRecycler();

      public:
        /**
         * if we keep N version tablets in disk, then
         * recycle all tablets has version = (current frozen memtable version - N)
         * call prepare_recycle  when decided recycle expired tablets.
         */
        int prepare_recycle(const int64_t version);


        /**
         * recycle all tablets has version = %version
         */
        int recycle(const int64_t version);

        /**
         * recycle specific tablet of %range 
         * 
         * @param range of tablet
         * @param version must equal to version of prepare_recycle
         */
        int recycle_tablet(const common::ObRange & range, const int64_t version);

        /**
         * backup all meta files with version in disks.
         */
        int backup_meta_files(const int64_t version);

      private:
        int load_all_tablets(const int64_t version);
        int load_tablet(const int64_t version, const int32_t disk_no);
        int check_current_status(const bool do_recycle);
        int do_recycle_tablet(const common::ObRange & range);
        
      private:
        ObTabletImage expired_image_;
        ObTabletManager& manager_;
    };

    /**
     * scan every disk directory, find expired meta index
     * files and sstable files for recycle.
     */
    class ObScanRecycler
    {
      public:
        ObScanRecycler(ObTabletManager& manager);
        ~ObScanRecycler();
      public:
        int recycle();
      private:
        int get_version(const char* idx_file, int64_t &data_version);
        int recycle_sstable(int32_t disk_no);
        int recycle_meta_file(int32_t disk_no);
      private:
        typedef bool (ObScanRecycler::*Predicate)(
            int32_t disk_no, const char* dir, const char * filename);
        typedef int (*Operate)(int32_t disk_no, 
            const char* dir, const char* filename);
        typedef int (*Filter)(const struct dirent* d);

        bool check_if_expired_meta_file(
            int32_t disk_no, const char* dir, const char* filename);
        bool check_if_expired_sstable(
            int32_t disk_no, const char* dir, const char* filename);

        static int do_recycle_file(
            int32_t disk_no, const char* dir, const char* filename);

        static int sstable_file_name_filter(const struct dirent* d);
        static int meta_file_name_filter(const struct dirent* d);

        int do_scan(int32_t disk_no, Filter filter, Predicate pred, Operate op);
      private:
        ObTabletManager& manager_;
    };

    class ObExpiredSSTablePool
    {
      public:
        ObExpiredSSTablePool();
        ~ObExpiredSSTablePool();
        int init();
        int scan();
        int get_expired_sstable(int32_t disk_no,char *path,int32_t size);
      private:
        int scan_disk(int32_t disk_no);
        static const int32_t MAX_FILES_NUM = 100;
        struct FileList
        {
          FileList():files_num(0),current_idx(0) {}
          char files[MAX_FILES_NUM][common::OB_MAX_FILE_NAME_LENGTH];
          int32_t files_num;
          int32_t current_idx;
        };
        bool inited_;
        FileList *files_list_;
        mutable tbsys::CThreadMutex lock_;
    };

  } /* chunkserver */
} /* oceanbase */
#endif
