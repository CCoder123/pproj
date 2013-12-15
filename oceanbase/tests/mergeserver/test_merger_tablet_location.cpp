/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * test_merger_tablet_location.cpp for ...
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
#include "ob_ms_tablet_location.h"

using namespace std;
using namespace oceanbase::common;
using namespace oceanbase::mergeserver;

int main(int argc, char **argv)
{
  ob_init_memory_pool();
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

class TestTabletLocation: public ::testing::Test
{
  public:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

const static int64_t timeout = 1000 * 1000 * 1000L;

TEST_F(TestTabletLocation, test_init)
{
  ObMergerTabletLocationCache cache;
  int ret = cache.init(1000, 100, timeout);
  EXPECT_TRUE(ret == OB_SUCCESS);
}


TEST_F(TestTabletLocation, test_size)
{
  ObMergerTabletLocationCache cache;
  EXPECT_TRUE(cache.size() == 0);
  int ret = cache.clear();
  EXPECT_TRUE(ret != OB_SUCCESS);

  ret = cache.init(1000, 100, timeout);
  EXPECT_TRUE(ret == OB_SUCCESS);
  EXPECT_TRUE(cache.size() == 0);
  ret = cache.clear();
  EXPECT_TRUE(ret == OB_SUCCESS);
  
  char temp[100];
  char temp_end[100];
  uint64_t count = 0;
  // not more than 10 bit rows because of the string key
  const uint64_t START_ROW = 10L;
  const uint64_t MAX_ROW = 79L;
  // set
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    ObServer server;
    server.set_ipv4_addr(i + 256, 1024 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));

    snprintf(temp, 100, "row_%lu", i);
    snprintf(temp_end, 100, "row_%lu", i + 10);
    ObString start_key(100, strlen(temp), temp);
    ObString end_key(100, strlen(temp_end), temp_end);

    ObRange range;
    range.table_id_ = 1;
    range.start_key_ = start_key;
    range.end_key_ = end_key;

    ret = cache.set(range, location);
    EXPECT_TRUE(OB_SUCCESS == ret);
    EXPECT_TRUE(cache.size() == ++count);
  }
  ret = cache.clear();
  EXPECT_TRUE(ret == OB_SUCCESS);
  EXPECT_TRUE(cache.size() == 0);
  
  // not find items
  for (uint64_t i = START_ROW; i < MAX_ROW; ++i)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    ObMergerTabletLocationList location;
    ret = cache.get(1, start_key, location);
    EXPECT_TRUE(ret != OB_SUCCESS);
  }
  
  // set again
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    ObServer server;
    server.set_ipv4_addr(i + 256, 1024 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));

    snprintf(temp, 100, "row_%lu", i);
    snprintf(temp_end, 100, "row_%lu", i + 10);
    ObString start_key(100, strlen(temp), temp);
    ObString end_key(100, strlen(temp_end), temp_end);

    ObRange range;
    range.table_id_ = 1;
    range.start_key_ = start_key;
    range.end_key_ = end_key;

    ret = cache.set(range, location);
    EXPECT_TRUE(OB_SUCCESS == ret);
  }
  
  // find in cache
  for (uint64_t i = START_ROW; i < MAX_ROW; ++i)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    ObMergerTabletLocationList location;
    ret = cache.get(1, start_key, location);
    EXPECT_TRUE(ret == OB_SUCCESS);
  }
  
  EXPECT_TRUE(cache.size() == count);
  ret = cache.clear();
  EXPECT_TRUE(ret == OB_SUCCESS);
  EXPECT_TRUE(cache.size() == 0);
  // not find
  for (uint64_t i = START_ROW; i < MAX_ROW; ++i)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    ObMergerTabletLocationList location;
    ret = cache.get(1, start_key, location);
    EXPECT_TRUE(ret != OB_SUCCESS);
  }
}


TEST_F(TestTabletLocation, test_set)
{
  ObMergerTabletLocationCache cache;
  int ret = cache.init(50000 * 5, 1000, 10000);
  EXPECT_TRUE(ret == OB_SUCCESS);

  char temp[100];
  char temp_end[100];
  uint64_t count = 0;
  ObMergerTabletLocationList temp_location;
  // warning
  // not more than 10 bit rows because of the string key
  const uint64_t START_ROW = 10L;
  const uint64_t MAX_ROW = 79L;
  // set
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    ObServer server;
    server.set_ipv4_addr(i + 256, 1024 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));

    snprintf(temp, 100, "row_%lu", i);
    snprintf(temp_end, 100, "row_%lu", i + 10);
    ObString start_key(100, strlen(temp), temp);
    ObString end_key(100, strlen(temp_end), temp_end);

    ObRange range;
    range.table_id_ = 1;
    range.start_key_ = start_key;
    range.end_key_ = end_key;

    ret = cache.set(range, location);
    EXPECT_TRUE(ret == OB_SUCCESS);

    // get location 
    ret = cache.get(1, end_key, temp_location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    EXPECT_TRUE(temp_location.get_timestamp() < tbsys::CTimeUtil::getTime());
    //printf("tablet_id[%lu]\n", temp_location.begin()->tablet_id_);
    
    // small data
    //EXPECT_TRUE(temp_location.begin()->tablet_id_ == i);
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_port() == (int32_t)(i + 1024));
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_ipv4() == (int32_t)(i + 256));
    printf("row_key[%s], host[%u]\n", temp, temp_location[0].server_.chunkserver_.get_ipv4());
    
    // overwrite
    ret = cache.set(range, location);
    EXPECT_TRUE(ret == OB_SUCCESS);

    // cache item count
    ++count;
    EXPECT_TRUE(cache.size() == count);
  }
  //
  EXPECT_TRUE(cache.size() == count);
  
  // get 
  for (uint64_t i = START_ROW; i < MAX_ROW; ++i)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    
    ret = cache.get(1, start_key, temp_location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    EXPECT_TRUE(temp_location.get_timestamp() < tbsys::CTimeUtil::getTime());
    //EXPECT_TRUE(temp_location.begin()->tablet_id_ == (i/10 * 10));
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_port() == (int32_t)(i/10 * 10 + 1024));
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_ipv4() == (int32_t)(i/10 * 10 + 256));
    printf("row_key[%s], host[%u]\n", temp, temp_location[0].server_.chunkserver_.get_ipv4());
  }
  
  // update
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    ObServer server;
    server.set_ipv4_addr(i + 255, 1023 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));

    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    location.set_timestamp(i);
    ret = cache.update(1, start_key, location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    
    // update not exist
    snprintf(temp, 100, "wor_%ld", i + 10);
    start_key.assign(temp, strlen(temp));

    ret = cache.update(1, start_key, location);
    EXPECT_TRUE(ret != OB_SUCCESS);
  }


  for (uint64_t i = START_ROW; i < MAX_ROW; ++i)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    ret = cache.get(1, start_key, temp_location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    //EXPECT_TRUE(temp_location.begin()->tablet_id_ == (i/10 * 10));
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_port() == (int32_t)(i/10 * 10 + 1023));
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_ipv4() == (int32_t)(i/10 * 10 + 255));
  }

  // del
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    
    printf("del rowkey[%s]\n", temp);
    ret = cache.del(1, start_key);
    EXPECT_TRUE(ret == OB_SUCCESS);
    
    --count;
    EXPECT_TRUE(cache.size() == count);
    
    ret = cache.del(2, start_key);
    EXPECT_TRUE(ret != OB_SUCCESS);
  }
  
  // after delete
  for (uint64_t i = START_ROW; i < MAX_ROW; ++i)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    //printf("get rowkey[%s]\n", temp);
    ret = cache.get(1, start_key, temp_location);
    EXPECT_TRUE(ret != OB_SUCCESS);
  }
}


TEST_F(TestTabletLocation, test_split)
{
  ObMergerTabletLocationCache cache;
  int ret = cache.init(50000 * 5, 1000, 10000);
  EXPECT_TRUE(ret == OB_SUCCESS);

  char temp[100];
  char temp_end[100];
  uint64_t count = 0;
  ObMergerTabletLocationList temp_location;
  // warning
  // not more than 10 bit rows because of the string key
  const uint64_t START_ROW = 10L;
  const uint64_t MAX_ROW = 85L;
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    ObServer server;
    server.set_ipv4_addr(i + 256, 1024 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));

    snprintf(temp, 100, "row_%lu", i);
    snprintf(temp_end, 100, "row_%lu", i + 10);
    ObString start_key(100, strlen(temp), temp);
    ObString end_key(100, strlen(temp_end), temp_end);

    ObRange range;
    range.table_id_ = 1;
    range.start_key_ = start_key;
    range.end_key_ = end_key;

    ret = cache.set(range, location);
    EXPECT_TRUE(ret == OB_SUCCESS);

    // get location 
    ret = cache.get(1, end_key, temp_location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    EXPECT_TRUE(temp_location.get_timestamp() < tbsys::CTimeUtil::getTime());
    
    // small data
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_port() == (int32_t)(i + 1024));
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_ipv4() == (int32_t)(i + 256));
    
    // overwrite
    ret = cache.set(range, location);
    EXPECT_TRUE(ret == OB_SUCCESS);

    // cache item count
    ++count;
    EXPECT_TRUE(cache.size() == count);
  }
  //
  EXPECT_TRUE(cache.size() == count);
  
  // split
  count = 0;
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 5)
  {
    ObServer server;
    server.set_ipv4_addr(i + 255, 1023 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    snprintf(temp, 100, "row_%lu", i);
    snprintf(temp_end, 100, "row_%lu", i + 5);
    ObString start_key(100, strlen(temp), temp);
    ObString end_key(100, strlen(temp_end), temp_end);
    ObRange range;
    range.table_id_ = 1;
    range.start_key_ = start_key;
    range.end_key_ = end_key;
    location.set_timestamp(i);
    ret = cache.set(range, location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    snprintf(temp, 100, "row_%ld", i+1);
    start_key.assign(temp, strlen(temp));
    ret = cache.get(1, start_key, temp_location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    ++count;
  }

  // just overwrite all
  for (uint64_t i = START_ROW; i < MAX_ROW; ++i)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    ret = cache.get(1, start_key, temp_location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    printf("expect[%ld:%ld], real[%d:%d]\n", (i/5 * 5 + 255), (i/5 * 5 + 1023), 
        temp_location[0].server_.chunkserver_.get_ipv4(), temp_location[0].server_.chunkserver_.get_port());
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_port() == (int32_t)(i/5 * 5 + 1023));
    EXPECT_TRUE(temp_location[0].server_.chunkserver_.get_ipv4() == (int32_t)(i/5 * 5 + 255));
  }
}

TEST_F(TestTabletLocation, test_pressure)
{
  ObMergerTabletLocationCache cache;
  int ret = cache.init(50000 * 5, 1000, 10000);
  EXPECT_TRUE(ret == OB_SUCCESS);

  char temp[100];
  char tempkey[100];
  char temp_end[100];
  snprintf(tempkey, 100, "rowkey_rowkey_12_rowkey_rowkey");
  ObString search_key(100, strlen(tempkey), tempkey);
  //ObString search_key(100, strlen(temp), temp);

  snprintf(temp, 100, "rowkey_rowkey_11_rowkey_rowkey");
  snprintf(temp_end, 100, "rowkey_rowkey_15_rowkey_rowkey");
  ObString start_key(100, strlen(temp), temp);
  ObString end_key(100, strlen(temp_end), temp_end);

  ObRange range;
  range.table_id_ = 1;
  range.start_key_ = start_key;
  range.end_key_ = end_key;
  ObMergerTabletLocationList location;

  // for debug cache memory leak
  int64_t count = 0;//1000000;
  while (count < 100)
  {
    ret = cache.set(range, location);
    EXPECT_TRUE(ret == OB_SUCCESS);
    ret = cache.del(1, search_key);
    EXPECT_TRUE(ret == OB_SUCCESS);
    ++count;
  }
}

static const uint64_t START = 10000L;
static const uint64_t MAX = 90000L;

void * del_routine(void * argv)
{
  ObMergerTabletLocationCache * cache = (ObMergerTabletLocationCache *) argv;
  EXPECT_TRUE(cache != NULL);

  char temp[100];
  const uint64_t START_ROW = START + (int64_t(pthread_self())%10 * 100);
  const uint64_t MAX_ROW = MAX;
  ObMergerTabletLocationList temp_location;
  // get 
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    cache->del(1, start_key);
  }
  return NULL;
}

void * get_routine(void * argv)
{
  ObMergerTabletLocationCache * cache = (ObMergerTabletLocationCache *) argv;
  EXPECT_TRUE(cache != NULL);

  char temp[100];
  const uint64_t START_ROW = START + (int64_t(pthread_self())%10 * 100);
  const uint64_t MAX_ROW = MAX;
  ObMergerTabletLocationList temp_location;
  // get 
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    cache->get(1, start_key, temp_location);
  }
  return NULL;
}

void * set_routine(void * argv)
{
  ObMergerTabletLocationCache * cache = (ObMergerTabletLocationCache *) argv;
  EXPECT_TRUE(cache != NULL);

  char temp[100];
  char temp_end[100];
  const uint64_t START_ROW = START + (int64_t(pthread_self())%10 * 100);
  const uint64_t MAX_ROW = MAX;
  // set
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    ObServer server;
    server.set_ipv4_addr(i + 256, 1024 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));

    snprintf(temp, 100, "row_%lu", i);
    snprintf(temp_end, 100, "row_%lu", i + 10);
    ObString start_key(100, strlen(temp), temp);
    ObString end_key(100, strlen(temp_end), temp_end);

    ObRange range;
    range.table_id_ = 1;
    range.start_key_ = start_key;
    range.end_key_ = end_key;

    int ret = cache->set(range, location);
    EXPECT_TRUE(ret == OB_SUCCESS);
  }
  return NULL;
}

void * update_routine(void * argv)
{
  ObMergerTabletLocationCache * cache = (ObMergerTabletLocationCache *) argv;
  EXPECT_TRUE(cache != NULL);
  char temp[100];
  const uint64_t START_ROW = START + (int64_t(pthread_self())%10 * 100);
  const uint64_t MAX_ROW = MAX;
  // update
  for (uint64_t i = START_ROW; i < MAX_ROW; i += 10)
  {
    ObServer server;
    server.set_ipv4_addr(i + 255, 1023 + i);
    ObTabletLocation addr(i, server);
    ObMergerTabletLocationList location;
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    EXPECT_TRUE(OB_SUCCESS == location.add(addr));
    
    snprintf(temp, 100, "row_%ld", i+1);
    ObString start_key(100, strlen(temp), temp);
    location.set_timestamp(i);

    cache->update(1, start_key, location);
  }
  return NULL;
}

#if false 
TEST_F(TestTabletLocation, test_thread)
{
  ObMergerTabletLocationCache cache;
  int ret = cache.init(50000 * 5, 1000, 10000);
  EXPECT_TRUE(ret == OB_SUCCESS);
  
  static const int THREAD_COUNT = 30;
  pthread_t threads[THREAD_COUNT][4];
  
  for (int i = 0; i < THREAD_COUNT; ++i)
  {
    ret = pthread_create(&threads[i][0], NULL, set_routine, &cache);
    EXPECT_TRUE(ret == OB_SUCCESS);
  }

  for (int i = 0; i < THREAD_COUNT; ++i)
  {
    ret = pthread_create(&threads[i][1], NULL, get_routine, &cache);
    EXPECT_TRUE(ret == OB_SUCCESS);
  }

  for (int i = 0; i < THREAD_COUNT; ++i)
  {
    ret = pthread_create(&threads[i][2], NULL, update_routine, &cache);
    EXPECT_TRUE(ret == OB_SUCCESS);
  }
  
  for (int i = 0; i < THREAD_COUNT; ++i)
  {
    ret = pthread_create(&threads[i][3], NULL, del_routine, &cache);
    EXPECT_TRUE(ret == OB_SUCCESS);
  } 

  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < THREAD_COUNT; ++j)
    {
      ret = pthread_join(threads[j][i], NULL);
    }
  }
}

#endif



