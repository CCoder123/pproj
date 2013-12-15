/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * test_merger_rpc_proxy.cpp for ...
 *
 * Authors:
 *   xielun <xielun.szd@taobao.com>
 *
 */
#include <iostream>
#include <sstream>
#include <algorithm>
#include <tblog.h>
#include <gtest/gtest.h>

#include "common/ob_schema.h"
#include "common/ob_malloc.h"
#include "common/ob_scanner.h"
#include "common/ob_tablet_info.h"
#include "ob_ms_rpc_proxy.h"
#include "ob_ms_tablet_location.h"
#include "ob_ms_schema_manager.h"
#include "ob_ms_rpc_stub.h"

#include "mock_server.h"
#include "mock_root_server.h"
#include "mock_update_server.h"
#include "mock_chunk_server.h"

using namespace std;
using namespace oceanbase::common;
using namespace oceanbase::mergeserver;
using namespace oceanbase::mergeserver::test;

const uint64_t timeout = 100000;
const char * addr = "localhost";

int main(int argc, char **argv)
{
  ob_init_memory_pool();
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

class TestRpcProxy: public ::testing::Test, public ObMergerRpcProxy
{
  public:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }

    static void * fetch_schema(void * argv)
    {
      ObMergerRpcProxy * proxy = (ObMergerRpcProxy *) argv;
      EXPECT_TRUE(NULL != proxy);
      int64_t timestamp = LOCAL_NEWEST;
      const ObSchemaManagerV2 * manager = NULL;
      EXPECT_TRUE(OB_SUCCESS == proxy->get_schema(timestamp, &manager));
      EXPECT_TRUE(NULL != manager);
      EXPECT_TRUE(manager->get_version() == 1022);

      // get new schema
      timestamp = 1022;
      EXPECT_TRUE(OB_SUCCESS == proxy->get_schema(timestamp, &manager));
      EXPECT_TRUE(NULL != manager);
      EXPECT_TRUE(manager->get_version() == 1025);
      return NULL;
    }
};

/*
// multi-thread schema test
TEST_F(TestRpcProxy, test_multi_schema)
{
  ObMergerRpcStub stub;
  ThreadSpecificBuffer buffer;
  ObPacketFactory factory;
  tbnet::Transport transport;
  tbnet::DefaultPacketStreamer streamer; 
  streamer.setPacketFactory(&factory);
  transport.start();
  ObClientManager client_manager;

  EXPECT_TRUE(OB_SUCCESS == client_manager.initialize(&transport, &streamer));
  EXPECT_TRUE(OB_SUCCESS == stub.init(&buffer, &client_manager));

  ObServer root_server;
  root_server.set_ipv4_addr(addr, MockRootServer::ROOT_SERVER_PORT);

  ObServer update_server;
  update_server.set_ipv4_addr(addr, MockUpdateServer::UPDATE_SERVER_PORT);
  
  ObServer merge_server;
  merge_server.set_ipv4_addr(addr, 10256);
  ObMergerRpcProxy proxy(0, timeout, root_server, update_server, merge_server);

  ObMergerSchemaManager * schema = new ObMergerSchemaManager;
  EXPECT_TRUE(NULL != schema);
  ObSchemaManagerV2 sample(1022);
  EXPECT_TRUE(OB_SUCCESS == schema->init(sample));

  ObMergerTabletLocationCache * location = new ObMergerTabletLocationCache;
  EXPECT_TRUE(NULL != location);

  // server not start
  EXPECT_TRUE(OB_SUCCESS == proxy.init(&stub, schema, location));

  // start root server
  MockRootServer server;
  MockServerRunner test_root_server(server);
  tbsys::CThread root_server_thread;
  root_server_thread.start(&test_root_server, NULL); 
  sleep(2);
  
  const int MAX_THREAD_COUNT = 15;
  pthread_t threads[MAX_THREAD_COUNT];
  for (int i = 0; i < MAX_THREAD_COUNT; ++i)
  {
    int ret = pthread_create(&threads[i], NULL, fetch_schema, &proxy);
    if (ret != OB_SUCCESS)
    {
      break;
    }
  }

  for (int i = 0; i < MAX_THREAD_COUNT; ++i)
  {
    pthread_join(threads[i], NULL);
  }
  
  delete schema;
  schema = NULL;
  delete location;
  location = NULL;

  transport.stop();
  server.stop();
  sleep(5);
}


TEST_F(TestRpcProxy, test_init)
{
  ObServer root_server;
  root_server.set_ipv4_addr(addr, MockRootServer::ROOT_SERVER_PORT);

  ObServer update_server;
  update_server.set_ipv4_addr(addr, MockUpdateServer::UPDATE_SERVER_PORT);

  ObServer merge_server;
  ObMergerRpcProxy proxy(0, timeout, root_server, update_server, merge_server);

  ObMergerRpcStub stub;
  EXPECT_TRUE(OB_SUCCESS != proxy.init(NULL, NULL, NULL));
  EXPECT_TRUE(OB_SUCCESS != proxy.init(&stub, NULL, NULL));

  ObMergerSchemaManager * schema = new ObMergerSchemaManager;
  EXPECT_TRUE(NULL != schema);
  EXPECT_TRUE(OB_SUCCESS != proxy.init(&stub, schema, NULL));

  ObMergerTabletLocationCache * location = new ObMergerTabletLocationCache;
  EXPECT_TRUE(NULL != location);

  EXPECT_TRUE(OB_SUCCESS == proxy.init(&stub, schema, location));
  EXPECT_TRUE(timeout == proxy.get_rpc_timeout());
  EXPECT_TRUE(root_server == proxy.get_root_server());
  EXPECT_TRUE(update_server == proxy.get_update_server());
  EXPECT_TRUE(NULL != proxy.get_rpc_stub());

  EXPECT_TRUE(OB_SUCCESS != proxy.init(&stub, schema, location));

  delete schema;
  schema = NULL;
  delete location;
  location = NULL;
}

TEST_F(TestRpcProxy, test_register)
{
  ObMergerRpcStub stub;
  ThreadSpecificBuffer buffer;
  ObPacketFactory factory;
  tbnet::Transport transport;
  tbnet::DefaultPacketStreamer streamer; 
  streamer.setPacketFactory(&factory);
  transport.start();
  ObClientManager client_manager;

  EXPECT_TRUE(OB_SUCCESS == client_manager.initialize(&transport, &streamer));
  EXPECT_TRUE(OB_SUCCESS == stub.init(&buffer, &client_manager));

  //
  ObServer root_server;
  root_server.set_ipv4_addr(addr, MockRootServer::ROOT_SERVER_PORT);

  ObServer update_server;
  update_server.set_ipv4_addr(addr, MockUpdateServer::UPDATE_SERVER_PORT);

  ObServer merge_server;
  merge_server.set_ipv4_addr(addr, 10256);
  ObMergerRpcProxy proxy(0, timeout, root_server, update_server, merge_server);

  // not init
  EXPECT_TRUE(OB_SUCCESS != proxy.register_merger(merge_server));

  ObMergerSchemaManager * schema = new ObMergerSchemaManager;
  EXPECT_TRUE(NULL != schema);

  ObMergerTabletLocationCache * location = new ObMergerTabletLocationCache;
  EXPECT_TRUE(NULL != location);

  // server not start
  EXPECT_TRUE(OB_SUCCESS == proxy.init(&stub, schema, location));
  EXPECT_TRUE(OB_SUCCESS != proxy.register_merger(merge_server));

  // start root server
  MockRootServer server;
  MockServerRunner test_root_server(server);
  tbsys::CThread root_server_thread;
  root_server_thread.start(&test_root_server, NULL); 
  sleep(2);

  // success 
  EXPECT_TRUE(OB_SUCCESS == proxy.register_merger(merge_server));

  delete schema;
  schema = NULL;
  delete location;
  location = NULL;

  transport.stop();
  server.stop();
  sleep(5);
}


TEST_F(TestRpcProxy, test_schema)
{
  ObMergerRpcStub stub;
  ThreadSpecificBuffer buffer;
  ObPacketFactory factory;
  tbnet::Transport transport;
  tbnet::DefaultPacketStreamer streamer; 
  streamer.setPacketFactory(&factory);
  transport.start();
  ObClientManager client_manager;

  EXPECT_TRUE(OB_SUCCESS == client_manager.initialize(&transport, &streamer));
  EXPECT_TRUE(OB_SUCCESS == stub.init(&buffer, &client_manager));

  ObServer root_server;
  root_server.set_ipv4_addr(addr, MockRootServer::ROOT_SERVER_PORT);

  ObServer update_server;
  update_server.set_ipv4_addr(addr, MockUpdateServer::UPDATE_SERVER_PORT);


  ObServer merge_server;
  merge_server.set_ipv4_addr(addr, 10256);
  ObMergerRpcProxy proxy(0, timeout, root_server, update_server, merge_server);

  // not init
  int64_t timestamp = 1234;
  EXPECT_TRUE(OB_SUCCESS != proxy.get_schema(timestamp, NULL));

  ObMergerSchemaManager * schema = new ObMergerSchemaManager;
  EXPECT_TRUE(NULL != schema);
  ObSchemaManagerV2 sample(1022);
  EXPECT_TRUE(OB_SUCCESS == schema->init(sample));

  ObMergerTabletLocationCache * location = new ObMergerTabletLocationCache;
  EXPECT_TRUE(NULL != location);

  // server not start
  EXPECT_TRUE(OB_SUCCESS == proxy.init(&stub, schema, location));
  EXPECT_TRUE(OB_SUCCESS != proxy.get_schema(timestamp, NULL));

  // start root server
  MockRootServer server;
  MockServerRunner test_root_server(server);
  tbsys::CThread root_server_thread;
  root_server_thread.start(&test_root_server, NULL); 
  sleep(2);

  // init schema manger
  timestamp = ObMergerRpcProxy::LOCAL_NEWEST;
  const ObSchemaManagerV2 * manager = NULL;
  EXPECT_TRUE(OB_SUCCESS == proxy.get_schema(timestamp, &manager));
  EXPECT_TRUE(NULL != manager);
  EXPECT_TRUE(manager->get_version() == 1022);

  // not exist but server exist
  timestamp = 1024;
  EXPECT_TRUE(OB_SUCCESS == proxy.get_schema(timestamp, &manager));
  EXPECT_TRUE(NULL != manager);
  // new schema finded
  EXPECT_TRUE(manager->get_version() == 1025);
  
  // get server newest for init
  timestamp = 100;
  // add schema failed
  EXPECT_TRUE(OB_SUCCESS != proxy.get_schema(timestamp, &manager));
  
  // local version
  timestamp = ObMergerRpcProxy::LOCAL_NEWEST;
  EXPECT_TRUE(OB_SUCCESS == proxy.get_schema(timestamp, &manager));
  EXPECT_TRUE(NULL != manager);
  EXPECT_TRUE(manager->get_version() == 1025);

  delete schema;
  schema = NULL;
  delete location;
  location = NULL;

  transport.stop();
  server.stop();
  sleep(5);
}
*/


TEST_F(TestRpcProxy, test_ups)
{
  ObMergerRpcStub stub;
  ThreadSpecificBuffer buffer;
  ObPacketFactory factory;
  tbnet::Transport transport;
  tbnet::DefaultPacketStreamer streamer; 
  streamer.setPacketFactory(&factory);
  transport.start();
  ObClientManager client_manager;

  EXPECT_TRUE(OB_SUCCESS == client_manager.initialize(&transport, &streamer));
  EXPECT_TRUE(OB_SUCCESS == stub.init(&buffer, &client_manager));

  ObServer root_server;
  root_server.set_ipv4_addr(addr, MockRootServer::ROOT_SERVER_PORT);

  ObServer update_server;
  update_server.set_ipv4_addr(addr, MockUpdateServer::UPDATE_SERVER_PORT);

  ObServer merge_server;
  merge_server.set_ipv4_addr(addr, 10256);
  ObMergerRpcProxy proxy(0, timeout, root_server, update_server, merge_server);

  // not init
  ObMergerTabletLocation cs_addr;
  ObGetParam get_param;
  ObString row_key;
  char * temp = "test";
  row_key.assign(temp, strlen(temp));
  ObCellInfo cell;
  cell.table_id_ = 234;
  cell.column_id_ = 111;
  cell.row_key_ = row_key;
  EXPECT_TRUE(OB_SUCCESS == get_param.add_cell(cell));

  ObScanner scanner;
  EXPECT_TRUE(OB_SUCCESS != proxy.ups_get(cs_addr, get_param, scanner));
  ObScanParam scan_param;
  EXPECT_TRUE(OB_SUCCESS != proxy.ups_scan(cs_addr, scan_param, scanner));

  // init
  ObMergerSchemaManager * schema = new ObMergerSchemaManager;
  EXPECT_TRUE(NULL != schema);
  ObMergerTabletLocationCache * location = new ObMergerTabletLocationCache;
  EXPECT_TRUE(NULL != location);

  // server not start
  EXPECT_TRUE(OB_SUCCESS == proxy.init(&stub, schema, location));

  EXPECT_TRUE(OB_SUCCESS != proxy.ups_get(cs_addr, get_param, scanner));
  EXPECT_TRUE(OB_SUCCESS != proxy.ups_scan(cs_addr, scan_param, scanner));

  // start root server
  MockUpdateServer server;
  MockServerRunner test_update_server(server);
  tbsys::CThread update_server_thread;
  update_server_thread.start(&test_update_server, NULL); 
  sleep(2);
  //
  EXPECT_TRUE(OB_SUCCESS == proxy.ups_get(cs_addr, get_param, scanner));
  uint64_t count = 0;
  ObScannerIterator iter;
  for (iter = scanner.begin(); iter != scanner.end(); ++iter)
  {
    EXPECT_TRUE(OB_SUCCESS == iter.get_cell(cell));
    printf("client:%.*s\n", cell.row_key_.length(), cell.row_key_.ptr());
    ++count;
  }
  // return 10 cells
  EXPECT_TRUE(count == 10);
  scanner.clear();

  ObScanParam param;
  ObRange range;
  range.border_flag_.set_min_value();
  ObString end_row;
  end_row.assign(temp, strlen(temp));
  //range.start_key_ = end_row;
  range.end_key_ = end_row;

  ObString name;
  name.assign(temp, strlen(temp));
  scan_param.set(OB_INVALID_ID, name, range);
  EXPECT_TRUE(OB_SUCCESS == proxy.ups_scan(cs_addr, scan_param, scanner));

  count = 0;
  for (iter = scanner.begin(); iter != scanner.end(); ++iter)
  {
    EXPECT_TRUE(OB_SUCCESS == iter.get_cell(cell));
    printf("client:%.*s\n", cell.row_key_.length(), cell.row_key_.ptr());
    ++count;
  }
  // return 10 cells
  EXPECT_TRUE(count == 10);

  delete schema;
  schema = NULL;
  delete location;
  location = NULL;

  transport.stop();
  server.stop();
  sleep(5);
}

TEST_F(TestRpcProxy, test_cs)
{
  ObMergerRpcStub stub;
  ThreadSpecificBuffer buffer;
  ObPacketFactory factory;
  tbnet::Transport transport;
  tbnet::DefaultPacketStreamer streamer; 
  streamer.setPacketFactory(&factory);
  transport.start();
  ObClientManager client_manager;

  EXPECT_TRUE(OB_SUCCESS == client_manager.initialize(&transport, &streamer));
  EXPECT_TRUE(OB_SUCCESS == stub.init(&buffer, &client_manager));

  ObServer root_server;
  root_server.set_ipv4_addr(addr, MockRootServer::ROOT_SERVER_PORT);

  ObServer update_server;
  update_server.set_ipv4_addr(addr, MockUpdateServer::UPDATE_SERVER_PORT);

  ObServer merge_server;
  merge_server.set_ipv4_addr(addr, 10256);
  ObMergerRpcProxy proxy(3, timeout, root_server, update_server, merge_server);

  // not init
  ObMergerTabletLocation cs_addr;
  ObGetParam get_param;
  ObCellInfo cell;
  ObString row_key;
  char temp[256] = "";
  snprintf(temp, 100, "row_100");
  row_key.assign(temp, strlen(temp));
  cell.table_id_ = 234;
  cell.column_id_ = 111;
  cell.row_key_ = row_key;
  EXPECT_TRUE(OB_SUCCESS == get_param.add_cell(cell));
  
  ObScanner scanner;
  EXPECT_TRUE(OB_SUCCESS != proxy.ups_get(cs_addr, get_param, scanner));
  ObScanParam scan_param;
  EXPECT_TRUE(OB_SUCCESS != proxy.ups_scan(cs_addr, scan_param, scanner));

  // init
  ObMergerSchemaManager * schema = new ObMergerSchemaManager;
  EXPECT_TRUE(NULL != schema);
  ObMergerTabletLocationCache * location = new ObMergerTabletLocationCache;
  EXPECT_TRUE(NULL != location);
  EXPECT_TRUE(OB_SUCCESS == location->init(50000 * 5, 1000, 10000));

  // init tablet cache 
  char temp_end[256] = "";
  ObServer server;
  const uint64_t START_ROW = 200L;
  const uint64_t MAX_ROW = 790L;
  for (uint64_t i = START_ROW; i < MAX_ROW - 100; i += 100)
  {
    server.set_ipv4_addr(i + 256, 1024 + i);
    ObTabletLocation addr(i, server);

    ObMergerTabletLocationList list;
    EXPECT_TRUE(OB_SUCCESS == list.add(addr));
    EXPECT_TRUE(OB_SUCCESS == list.add(addr));
    EXPECT_TRUE(OB_SUCCESS == list.add(addr));

    snprintf(temp, 100, "row_%lu", i);
    snprintf(temp_end, 100, "row_%lu", i + 100);
    ObString start_key(100, strlen(temp), temp);
    ObString end_key(100, strlen(temp_end), temp_end);

    ObRange range;
    range.table_id_ = 234;
    range.start_key_ = start_key;
    range.end_key_ = end_key;
    list.set_timestamp(tbsys::CTimeUtil::getTime()); 
    EXPECT_TRUE(OB_SUCCESS == location->set(range, list));
    ObString rowkey;
    snprintf(temp, 100, "row_%lu", i + 40);
    rowkey.assign(temp, strlen(temp));
    EXPECT_TRUE(OB_SUCCESS == location->get(234, rowkey, list));
    
    rowkey.assign(temp_end, strlen(temp_end));
    EXPECT_TRUE(OB_SUCCESS == location->get(234, rowkey, list));
  }

  // server not start
  ObIterator *it = NULL;
  ObMergerTabletLocation succ_addr;
  ObMergerTabletLocationList list;
  EXPECT_TRUE(OB_SUCCESS == proxy.init(&stub, schema, location));
  EXPECT_TRUE(OB_SUCCESS != proxy.cs_get(get_param, succ_addr, scanner, it));
  EXPECT_TRUE(OB_SUCCESS != proxy.cs_scan(scan_param, succ_addr, scanner, it));

  // start root server
  MockRootServer root;
  tbsys::CThread root_server_thread;
  MockServerRunner test_root_server(root);
  root_server_thread.start(&test_root_server, NULL); 

  // start chunk server
  MockChunkServer chunk;
  tbsys::CThread chunk_server_thread;
  MockServerRunner test_chunk_server(chunk);
  chunk_server_thread.start(&test_chunk_server, NULL); 
  sleep(2);

  // init get param
  snprintf(temp, 100, "row_105");
  row_key.assign(temp, strlen(temp));
  cell.table_id_ = 234;
  cell.column_id_ = 111;
  cell.row_key_ = row_key;
  get_param.add_cell(cell);

  // cache server not exist so fetch from root server error 
  ObMergerTabletLocationList addr_temp;
  EXPECT_TRUE(OB_SUCCESS != location->get(234, row_key, addr_temp));

  // scan from root server
  EXPECT_TRUE(OB_SUCCESS == proxy.cs_get(get_param, succ_addr, scanner, it));

  // delete the cache item and fetch from root server
  EXPECT_TRUE(OB_SUCCESS == location->get(234, row_key, addr_temp));

  ObGetParam get_param2;
  ObMergerTabletLocationList list2;
  // cache not exist, get from root server
  snprintf(temp, 100, "row_998");
  ObString row_key2;
  row_key2.assign(temp, strlen(temp));
  ObCellInfo cell2;
  cell2.table_id_ = 234;
  cell2.column_id_ = 111;
  cell2.row_key_ = row_key2;
  get_param2.add_cell(cell2);
  EXPECT_TRUE(OB_SUCCESS == proxy.cs_get(get_param2, succ_addr, scanner,it));

  // not find the cs 
  snprintf(temp, 100, "zzz_row_999");
  EXPECT_TRUE(OB_SUCCESS != proxy.cs_get(get_param2, succ_addr, scanner,it));

  // ok
  snprintf(temp, 100, "row_%lu", MAX_ROW - 10);
  EXPECT_TRUE(OB_SUCCESS == proxy.cs_get(get_param2, succ_addr, scanner,it));

  uint64_t count = 0;
  ObScannerIterator iter;
  for (iter = scanner.begin(); iter != scanner.end(); ++iter)
  {
    EXPECT_TRUE(OB_SUCCESS == iter.get_cell(cell));
    printf("client:%.*s\n", cell.row_key_.length(), cell.row_key_.ptr());
    ++count;
  }
  scanner.clear();

  root.stop();
  chunk.stop();
  sleep(10);
  transport.stop();

  delete schema;
  schema = NULL;
  delete location;
  location = NULL;
}




