/**
 * (C) 2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  test_tablet_manager.cpp for test tablet manager
 *
 * Authors:
 *   chuanhui <rizhao.ych@taobao.com>
 *   huating <huating.zmq@taobao.com>
 *  
 */
#include <tblog.h>
#include <gtest/gtest.h>
#include "ob_chunk_server_main.h"
#include "mock_root_server.h"
#include "test_helper.h"
#include "ob_scanner.h"

using namespace std;
using namespace oceanbase::common;
using namespace oceanbase::chunkserver;


namespace oceanbase
{
  namespace tests
  {
    namespace chunkserver
    {
      static ObTabletManager tablet_manager;
      static TabletManagerIniter manager_initer(tablet_manager);
      
      class TestTabletManager : public ::testing::Test
      {
      public:
        static void SetUpTestCase()
        {
          int err = OB_SUCCESS;
          err = manager_initer.init(true);
          ASSERT_EQ(OB_SUCCESS, err);
        }
      
        static void TearDownTestCase()
        {
        }

      public:
        virtual void SetUp()
        {
      
        }
      
        virtual void TearDown()
        {

        }

        void tablet_scan_and_check(ObTabletManager& tablet_mgr, int64_t index)
        {
          int err = OB_SUCCESS;
          ObScanParam scan_param;
          ObRange scan_range;
          ObCellInfo** cell_infos;
          ObCellInfo ci;
          ObCellInfo expected;
          ObString table_name(strlen("sstable") + 1, strlen("sstable") + 1, "sstable");
  
          cell_infos = manager_initer.get_mult_sstable_builder().get_cell_infos(index);
          scan_range.table_id_ = SSTableBuilder::table_id;
          scan_range.start_key_ = cell_infos[0][0].row_key_;
          scan_range.end_key_ = 
            cell_infos[SSTableBuilder::ROW_NUM - 1][SSTableBuilder::COL_NUM - 1].row_key_;
          scan_range.border_flag_.set_inclusive_start();
          scan_range.border_flag_.set_inclusive_end();
          
          scan_param.set(SSTableBuilder::table_id, table_name, scan_range);
          for (int64_t j = 0; j < SSTableBuilder::COL_NUM; ++j)
          {
            scan_param.add_column(cell_infos[0][j].column_id_);
          }
          
          ObScanner scanner;
          err = tablet_mgr.scan(scan_param, scanner);
          EXPECT_EQ(OB_SUCCESS, err);
  
          // check result
          int64_t count = 0;
          ObScannerIterator iter;
          for (iter = scanner.begin(); iter != scanner.end(); iter++)
          {
            expected = cell_infos[count / SSTableBuilder::COL_NUM][count % SSTableBuilder::COL_NUM];
            expected.table_name_ = table_name;
            EXPECT_EQ(OB_SUCCESS, iter.get_cell(ci));
            check_cell(expected, ci);
            ++count;
          }
          EXPECT_EQ(SSTableBuilder::ROW_NUM * SSTableBuilder::COL_NUM, count);
        }

        void tablet_get_and_check(ObTabletManager& tablet_mgr, int64_t index) 
        {
          int ret = OB_SUCCESS;
          ObGetParam get_param;
          ObCellInfo param_cell;
          ObScanner scanner;
          ObCellInfo** cell_infos;
          ObCellInfo ci;
          ObCellInfo expected;
          ObString table_name(strlen("sstable") + 1, strlen("sstable") + 1, "sstable");;

          cell_infos = manager_initer.get_mult_sstable_builder().get_cell_infos(index);
          for (int i = 0; i < SSTableBuilder::ROW_NUM; i++)
          {
            param_cell = cell_infos[i][0];
            param_cell.column_id_ = 0;
            ret = get_param.add_cell(param_cell);
            EXPECT_EQ(OB_SUCCESS, ret);
          }

          ret = tablet_mgr.get(get_param, scanner);
          ASSERT_EQ(OB_SUCCESS, ret);

          // check result
          int64_t count = 0;
          ObScannerIterator iter;
          for (iter = scanner.begin(); iter != scanner.end(); iter++)
          {
            expected = cell_infos[count / SSTableBuilder::COL_NUM][count % SSTableBuilder::COL_NUM];
            expected.table_name_ = table_name;
            EXPECT_EQ(OB_SUCCESS, iter.get_cell(ci));
            check_cell(expected, ci);
            ++count;
          }
          EXPECT_EQ(SSTableBuilder::ROW_NUM * SSTableBuilder::COL_NUM, count);
        }

        void tablet_get_with_mult_tablets(ObTabletManager& tablet_mgr, 
                                          int64_t row_index = 0,
                                          int64_t row_count_each = 1) 
        {
          int ret = OB_SUCCESS;
          ObGetParam get_param;
          ObCellInfo param_cell;
          ObScanner scanner;
          ObCellInfo** cell_infos;
          ObCellInfo ci;
          ObCellInfo expected;
          ObString table_name(strlen("sstable") + 1, strlen("sstable") + 1, "sstable");;

          for (int j = 0; j < MultSSTableBuilder::SSTABLE_NUM; j++)
          {
            cell_infos = manager_initer.get_mult_sstable_builder().get_cell_infos(j);
            for (int i = row_index; i < row_index + row_count_each; i++)
            {
              param_cell = cell_infos[i][0];
              param_cell.column_id_ = 0;
              ret = get_param.add_cell(param_cell);
              EXPECT_EQ(OB_SUCCESS, ret);
            }
          }

          ret = tablet_mgr.get(get_param, scanner);
          ASSERT_EQ(OB_SUCCESS, ret);

          // check result
          int64_t count = 0;
          int64_t sstable_index = 0;
          int64_t index_i = 0;
          int64_t sstable_columns = row_count_each * SSTableBuilder::COL_NUM;
          ObScannerIterator iter;
          for (iter = scanner.begin(); iter != scanner.end(); iter++)
          {
            sstable_index = count / sstable_columns;
            cell_infos = manager_initer.get_mult_sstable_builder()
                         .get_cell_infos(sstable_index);
            index_i = row_index + (count - sstable_index * sstable_columns) / SSTableBuilder::COL_NUM;
            expected = cell_infos[index_i][count % SSTableBuilder::COL_NUM];
            expected.table_name_ = table_name;
            EXPECT_EQ(OB_SUCCESS, iter.get_cell(ci));
            check_cell(expected, ci);
            ++count;
          }
          EXPECT_EQ(row_count_each * SSTableBuilder::COL_NUM 
                    * MultSSTableBuilder::SSTABLE_NUM, count);
        }
      };
      
      TEST_F(TestTabletManager, test_load_tablets)
      {
        int err = OB_SUCCESS;
        ObTabletManager tablet_mgr;
        TabletManagerIniter initer(tablet_mgr);
        int32_t disk_no_array[MultSSTableBuilder::DISK_NUM];

        err = initer.init();
        ASSERT_EQ(OB_SUCCESS, err);
      
        for (int i = 0; i < MultSSTableBuilder::DISK_NUM; i++)
        {
          disk_no_array[i] = i + 1;
        }
        err = tablet_mgr.load_tablets(disk_no_array, MultSSTableBuilder::DISK_NUM);
        ASSERT_EQ(OB_SUCCESS, err);

        for (int i = 0; i < MultSSTableBuilder::SSTABLE_NUM; i++)
        {
          tablet_scan_and_check(tablet_manager, i);
          tablet_scan_and_check(tablet_mgr, i);
          tablet_get_and_check(tablet_manager, i);
          tablet_get_and_check(tablet_mgr, i);
        }
      }

      TEST_F(TestTabletManager, test_get_with_mult_tablets)
      {
        int err = OB_SUCCESS;
        ObTabletManager tablet_mgr;
        TabletManagerIniter initer(tablet_mgr);
        int32_t disk_no_array[MultSSTableBuilder::DISK_NUM];

        err = initer.init();
        ASSERT_EQ(OB_SUCCESS, err);
      
        for (int i = 0; i < MultSSTableBuilder::DISK_NUM; i++)
        {
          disk_no_array[i] = i + 1;
        }
        err = tablet_mgr.load_tablets(disk_no_array, MultSSTableBuilder::DISK_NUM);
        ASSERT_EQ(OB_SUCCESS, err);

        tablet_get_with_mult_tablets(tablet_manager, 0, 1);
        tablet_get_with_mult_tablets(tablet_manager, SSTableBuilder::ROW_NUM / 2, 1);
        tablet_get_with_mult_tablets(tablet_manager, SSTableBuilder::ROW_NUM - 1, 1);

        tablet_get_with_mult_tablets(tablet_mgr, 0, 1);
        tablet_get_with_mult_tablets(tablet_mgr, SSTableBuilder::ROW_NUM / 2, 1);
        tablet_get_with_mult_tablets(tablet_mgr, SSTableBuilder::ROW_NUM - 1, 1);
      }
      
      TEST_F(TestTabletManager, test_report_tablets)
      {
        int err = OB_SUCCESS;
        ObTabletManager tablet_mgr;
        const char* dst_addr = "localhost";
        TabletManagerIniter initer(tablet_mgr);
        int32_t disk_no_array[MultSSTableBuilder::DISK_NUM];

        err = initer.init();
        ASSERT_EQ(OB_SUCCESS, err);
        for (int i = 0; i < MultSSTableBuilder::DISK_NUM; i++)
        {
          disk_no_array[i] = i + 1;
        }
        err = tablet_mgr.load_tablets(disk_no_array, MultSSTableBuilder::DISK_NUM);
        ASSERT_EQ(OB_SUCCESS, err);
              
        ObChunkServerMain* cm = ObChunkServerMain::get_instance();
        ObChunkServer& cs = cm->get_chunk_server();
        // init global root server
        ObServer& root_server = const_cast<ObServer&> (cs.get_root_server());
        root_server.set_ipv4_addr(dst_addr, MOCK_SERVER_LISTEN_PORT);
    
        // init global client manager
        ObClientManager& client_manager = const_cast<ObClientManager&> (cs.get_client_manager());
        ObPacketFactory factory;
        tbnet::Transport transport;
        tbnet::DefaultPacketStreamer streamer;
      
        streamer.setPacketFactory(&factory);
        EXPECT_EQ(OB_SUCCESS, client_manager.initialize(&transport, &streamer));
        transport.start();
      
        tbsys::CThread test_root_server_thread;
        MockRootServerRunner test_root_server;
        test_root_server_thread.start(&test_root_server, NULL);

        sleep(1);

        err = tablet_mgr.report_tablets();
        EXPECT_EQ(0, err);

        transport.stop();
        transport.wait();
      }

    } // end namespace chunkserver
  } // end namespace tests
} // end namespace oceanbase

class FooEnvironment : public testing::Environment
{
  public:
    virtual void SetUp()
    {
      TBSYS_LOGGER.setLogLevel("ERROR");
      ob_init_memory_pool();
      prepare_sstable_directroy(10);
    }
    virtual void TearDown()
    {
    }
};

int main(int argc, char** argv)
{
  testing::AddGlobalTestEnvironment(new FooEnvironment);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
