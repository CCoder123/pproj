/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * test_merger_tablet_location_item.cpp for ...
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
#include "ob_ms_tablet_location_item.h"

using namespace std;
using namespace oceanbase::common;
using namespace oceanbase::mergeserver;

int main(int argc, char **argv)
{
  ob_init_memory_pool();
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

class TestTabletLocationItem: public ::testing::Test
{
  public:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestTabletLocationItem, test_add)
{
  ObTabletLocation server;
  ObMergerTabletLocationList list;
  EXPECT_TRUE(0 == list.size());
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    server.tablet_version_ = i;
    EXPECT_TRUE(OB_SUCCESS == list.add(server));
    EXPECT_TRUE(i + 1 == list.size());
  }
  EXPECT_TRUE(OB_SUCCESS != list.add(server));
  EXPECT_TRUE(OB_SUCCESS != list.add(server));
  EXPECT_TRUE(OB_SUCCESS != list.add(server));
  EXPECT_TRUE(ObMergerTabletLocationList::MAX_REPLICA_COUNT == list.size());
}


TEST_F(TestTabletLocationItem, test_valid)
{
  ObMergerTabletLocationList list;
  ObTabletLocation temp_server;
  ObServer chunkserver;
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    chunkserver.set_ipv4_addr(i + 300000, i + 1024);
    temp_server.tablet_version_ = i;
    temp_server.chunkserver_ = chunkserver;
    EXPECT_TRUE(OB_SUCCESS == list.add(temp_server));
  }
  
  // all invalid 
  ObMergerTabletLocation server;
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    chunkserver.set_ipv4_addr(i + 300000, i + 1024);
    server.server_.chunkserver_ = chunkserver;
    server.server_.tablet_version_ = i;
    EXPECT_TRUE(OB_SUCCESS == list.set_item_invalid(server));
    EXPECT_TRUE(list[i].err_times_ == ObMergerTabletLocation::MAX_ERR_TIMES);
    EXPECT_TRUE(list.get_valid_count() == ObMergerTabletLocationList::MAX_REPLICA_COUNT - i - 1);
  }

  list.set_item_valid(1100);
  EXPECT_TRUE(list.get_timestamp() == 1100);
  EXPECT_TRUE(list.get_valid_count() == ObMergerTabletLocationList::MAX_REPLICA_COUNT);
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    EXPECT_TRUE(list[i].err_times_ == 0);
  }
}

TEST_F(TestTabletLocationItem, test_invalid)
{
  ObMergerTabletLocationList list;
  EXPECT_TRUE(list.get_valid_count() == 0);
  ObTabletLocation temp_server;
  ObServer chunkserver;
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    chunkserver.set_ipv4_addr(i + 300000, i + 1024);
    temp_server.tablet_version_ = i;
    temp_server.chunkserver_ = chunkserver;
    EXPECT_TRUE(OB_SUCCESS == list.add(temp_server));
    EXPECT_TRUE(list.get_valid_count() == i + 1);
  }

  EXPECT_TRUE(list.get_valid_count() == ObMergerTabletLocationList::MAX_REPLICA_COUNT);

  ObMergerTabletLocation server;

  // not exist
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    chunkserver.set_ipv4_addr(i + 200000, i + 1024);
    server.server_.chunkserver_ = chunkserver;
    server.server_.tablet_version_ = i;
    EXPECT_TRUE(OB_SUCCESS != list.set_item_invalid(server));
    EXPECT_TRUE(list[i].err_times_ != ObMergerTabletLocation::MAX_ERR_TIMES);
    EXPECT_TRUE(list.get_valid_count() == ObMergerTabletLocationList::MAX_REPLICA_COUNT);
  }

  // set second invalid
  chunkserver.set_ipv4_addr(1 + 300000, 1 + 1024);
  server.server_.chunkserver_ = chunkserver;
  server.server_.tablet_version_ = 1;
  EXPECT_TRUE(OB_SUCCESS == list.set_item_invalid(server));
  EXPECT_TRUE(list.get_valid_count() == ObMergerTabletLocationList::MAX_REPLICA_COUNT - 1);
  EXPECT_TRUE(list[1].err_times_ == ObMergerTabletLocation::MAX_ERR_TIMES);
  EXPECT_TRUE(list[0].err_times_ != ObMergerTabletLocation::MAX_ERR_TIMES);
  EXPECT_TRUE(list[2].err_times_ != ObMergerTabletLocation::MAX_ERR_TIMES);

  // all invalid 
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    chunkserver.set_ipv4_addr(i + 300000, i + 1024);
    server.server_.chunkserver_ = chunkserver;
    server.server_.tablet_version_ = i;
    EXPECT_TRUE(OB_SUCCESS == list.set_item_invalid(server));
    EXPECT_TRUE(list[i].err_times_ == ObMergerTabletLocation::MAX_ERR_TIMES);
  }

  EXPECT_TRUE(list.get_valid_count() == 0);

  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    server.server_.tablet_version_ = i + 100;
    EXPECT_TRUE(OB_SUCCESS != list.set_item_invalid(server));
  }
}


TEST_F(TestTabletLocationItem, test_del)
{
  ObMergerTabletLocation server;
  ObMergerTabletLocationList list;
  EXPECT_TRUE(OB_SUCCESS != list.del(0, server));
  EXPECT_TRUE(OB_SUCCESS != list.del(1, server));
  EXPECT_TRUE(OB_SUCCESS != list.del(2, server));
  EXPECT_TRUE(OB_SUCCESS != list.del(3, server));
  ObTabletLocation temp_server;
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    temp_server.tablet_version_ = i;
    EXPECT_TRUE(OB_SUCCESS == list.add(temp_server));
    EXPECT_TRUE(i + 1 == list.size());
  }

  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    EXPECT_TRUE(OB_SUCCESS == list.del(0, server));
    EXPECT_TRUE(0 == server.err_times_);
    EXPECT_TRUE(i == server.server_.tablet_version_);
    EXPECT_TRUE(ObMergerTabletLocationList::MAX_REPLICA_COUNT - i - 1 == list.size());
  }
  
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    temp_server.tablet_version_ = i;
    EXPECT_TRUE(OB_SUCCESS == list.add(temp_server));
    EXPECT_TRUE(i + 1 == list.size());
  }

  for (int64_t i = ObMergerTabletLocationList::MAX_REPLICA_COUNT - 1; i >= 0; --i)
  {
    EXPECT_TRUE(OB_SUCCESS == list.del(0, server));
    EXPECT_TRUE(ObMergerTabletLocationList::MAX_REPLICA_COUNT - i - 1 == server.server_.tablet_version_);
    EXPECT_TRUE(i == list.size());
  }

  EXPECT_TRUE(0 == list.size());
  
  for (int64_t i = ObMergerTabletLocationList::MAX_REPLICA_COUNT - 1; i >= 0; --i)
  {
    EXPECT_TRUE(OB_SUCCESS != list.del(0, server));
  }

  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    temp_server.tablet_version_ = i;
    EXPECT_TRUE(OB_SUCCESS == list.add(temp_server));
    EXPECT_TRUE(i + 1 == list.size());
  }
  
  EXPECT_TRUE(OB_SUCCESS == list.del(1, server));
  EXPECT_TRUE(1 == server.server_.tablet_version_);
  EXPECT_TRUE(ObMergerTabletLocationList::MAX_REPLICA_COUNT - 1 == list.size());
}

TEST_F(TestTabletLocationItem, test_timestamp)
{
  ObMergerTabletLocationList list;
  EXPECT_TRUE(list.get_timestamp() == 0);
  list.set_timestamp(1025);
  EXPECT_TRUE(list.get_timestamp() == 1025);
  list.set_timestamp(1026);
  EXPECT_TRUE(list.get_timestamp() == 1026);
}

TEST_F(TestTabletLocationItem, test_serialize)
{
  ObMergerTabletLocation server;
  ObMergerTabletLocationList list;
  list.set_timestamp(1000); 
  ObTabletLocation temp_server;
  ObServer chunkserver;
  for (int64_t i = 0; i < ObMergerTabletLocationList::MAX_REPLICA_COUNT; ++i)
  {
    chunkserver.set_ipv4_addr(i + 300000, i + 1024);
    temp_server.tablet_version_ = i;
    temp_server.chunkserver_ = chunkserver;
    EXPECT_TRUE(OB_SUCCESS == list.add(temp_server));
    EXPECT_TRUE(i + 1 == list.size());
  }
  
  list.print_info();
  int64_t size = list.get_serialize_size();
  EXPECT_TRUE(size != 0);
  
  char * temp = new char[size];
  EXPECT_TRUE(NULL != temp);
  int64_t pos = 0;
  EXPECT_TRUE(OB_SUCCESS != list.serialize(temp, size - 1 , pos));
  pos = 0;
  EXPECT_TRUE(OB_SUCCESS == list.serialize(temp, size, pos));
  EXPECT_TRUE(pos == size);
  
  ObMergerTabletLocationList list2;
  pos = 0;
  EXPECT_TRUE(OB_SUCCESS == list2.deserialize(temp, size, pos));
  list2.print_info();
  list.print_info();
}


TEST_F(TestTabletLocationItem, test_sort)
{
  ObMergerTabletLocationList list;
  ObServer root_server;
  const char * addr = "localhost";
  root_server.set_ipv4_addr(addr, 8888);
  int32_t server_ip = root_server.get_ipv4();
  
  // one server
  ObTabletLocation location;
  srand(100);
  ObServer temp;
  temp.set_ipv4_addr(rand() % 10245, 1023);
  //location.tablet_id_ = 1;
  location.chunkserver_ = temp;
  list.add(location);
  EXPECT_TRUE(OB_SUCCESS == list.sort(root_server));
  
  int64_t count = list.size();
  for (int64_t i = 0; i < count - 1; ++i)
  {
    EXPECT_TRUE(abs(list[i].server_.chunkserver_.get_ipv4() - server_ip) 
        <= abs(list[i+1].server_.chunkserver_.get_ipv4() - server_ip));
  } 

  // two server
  temp.set_ipv4_addr(rand() % 10245, 1024);
  location.chunkserver_ = temp;
  list.add(location);
  EXPECT_TRUE(OB_SUCCESS == list.sort(root_server));
  
  count = list.size();
  for (int64_t i = 0; i < count - 1; ++i)
  {
    EXPECT_TRUE(abs(list[i].server_.chunkserver_.get_ipv4() - server_ip) 
        <= abs(list[i+1].server_.chunkserver_.get_ipv4() - server_ip));
  } 
  
  // three server
  location.chunkserver_ = root_server;
  list.add(location);
  EXPECT_TRUE(OB_SUCCESS == list.sort(root_server));
  
  count = list.size();
  for (int64_t i = 0; i < count - 1; ++i)
  {
    EXPECT_TRUE(abs(list[i].server_.chunkserver_.get_ipv4() - server_ip) 
        <= abs(list[i+1].server_.chunkserver_.get_ipv4() - server_ip));
  } 
}






