/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * test_tablet_info_manager.cpp for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */


#include <gtest/gtest.h>
#include <unistd.h>
#include "common/ob_malloc.h"
#include "common/ob_vector.h"
#include "rootserver/ob_tablet_info_manager.h"
#include "rootserver/ob_root_meta2.h"
using namespace oceanbase::common;
using namespace oceanbase::rootserver;
namespace 
{
  void build_range(ObRange & r, int64_t tid, int8_t flag, const char* sk, const char* ek)
  {

    ObString start_key(strlen(sk), strlen(sk), (char*)sk);
    ObString end_key(strlen(ek), strlen(ek), (char*)ek);

    r.table_id_ = tid;
    r.border_flag_.set_data(flag);
    r.start_key_ = start_key;
    r.end_key_ = end_key;

  }
}

TEST(TabletInfoManagerTest, test)
{
  ObRange r1, r2, r3, r4;
  const char* key1 = "foo1";
  const char* key2 = "key2";
  const char* key3 = "too3";
  const char* key4 = "woo4";

  uint64_t table1 = 20;
  uint64_t table2 = 40;


  build_range(r1, table1, ObBorderFlag::INCLUSIVE_START, key1, key2);
  build_range(r2, table1, ObBorderFlag::INCLUSIVE_START, key2, key3);
  build_range(r3, table1, ObBorderFlag::INCLUSIVE_START|ObBorderFlag::MAX_VALUE, key3, key4);
  build_range(r4, table2, ObBorderFlag::INCLUSIVE_START|ObBorderFlag::INCLUSIVE_END|ObBorderFlag::MAX_VALUE, key1, key4);

  ObTabletInfo t1(r1, 0, 0);
  ObTabletInfo t2(r2, 0, 0);
  ObTabletInfo t3(r3, 0, 0);
  ObTabletInfo t4(r4, 0, 0);

  ObTabletInfoManager* tester = new ObTabletInfoManager();
  int32_t index = -1;
  ASSERT_EQ(OB_SUCCESS, tester->add_tablet_info(t1, index));
  ASSERT_EQ(0, index);

  ASSERT_EQ(OB_SUCCESS, tester->add_tablet_info(t2, index));
  ASSERT_EQ(1, index);

  ObTabletInfo* ret = tester->get_tablet_info(1);
  ASSERT_TRUE(ret->equal(t2));
  ASSERT_TRUE(1 == tester->get_index(ret));
  int s = 0;
  for (ret = tester->begin(); ret != tester->end(); ret ++)
  {
    s++;
  }
  ASSERT_EQ(2, s);
  ASSERT_EQ(2, tester->get_array_size());

  delete tester;
}
TEST(TabletInfoManagerTest, compare)
{
  ObRange r1, r2, r3, r4;
  const char* key1 = "foo1";
  const char* key2 = "key2";
  const char* key3 = "too3";
  const char* key4 = "woo4";

  uint64_t table1 = 20;
  uint64_t table2 = 40;


  build_range(r1, table1, ObBorderFlag::INCLUSIVE_START, key1, key2);
  build_range(r2, table1, ObBorderFlag::INCLUSIVE_START, key2, key3);
  build_range(r3, table1, ObBorderFlag::INCLUSIVE_START|ObBorderFlag::MAX_VALUE, key3, key4);
  build_range(r4, table2, ObBorderFlag::INCLUSIVE_START|ObBorderFlag::INCLUSIVE_END|ObBorderFlag::MAX_VALUE, key1, key4);

  ObTabletInfo t1(r1, 0, 0);
  ObTabletInfo t2(r2, 0, 0);
  ObTabletInfo t3(r3, 0, 0);
  ObTabletInfo t4(r4, 0, 0);

  ObTabletInfoManager* tester = new ObTabletInfoManager();
  int32_t index = -1;
  ASSERT_EQ(OB_SUCCESS, tester->add_tablet_info(t1, index));

  ASSERT_EQ(OB_SUCCESS, tester->add_tablet_info(t2, index));
  ASSERT_EQ(OB_SUCCESS, tester->add_tablet_info(t1, index));
  ASSERT_EQ(OB_SUCCESS, tester->add_tablet_info(t3, index));

  ObRootMeta2CompareHelper helper(tester);
  ASSERT_TRUE(helper.compare(0,1) < 0);
  ASSERT_TRUE(helper.compare(1,0) > 0);
  ASSERT_TRUE(helper.compare(2,0) == 0);
  ASSERT_TRUE(helper.compare(0,2) == 0);

  delete tester;
}

int main(int argc, char** argv)
{
  ob_init_memory_pool();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



