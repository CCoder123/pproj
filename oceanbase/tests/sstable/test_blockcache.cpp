/**
 * (C) 2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * test_blockcache.cpp for test block cache 
 *
 * Authors: 
 *   yubai< yubai.lk@taobao.com>
 *   huating <huating.zmq@taobao.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <new>
#include "gtest/gtest.h"
#include "sstable/ob_blockcache.h"
#include "chunkserver/ob_fileinfo_cache.h"
#include "sstable/ob_block_index_cache.h"
#include "sstable/ob_disk_path.h"

using namespace oceanbase;
using namespace chunkserver;
using namespace common;
using namespace sstable;

static uint64_t talbe_id = 1024;
static char file_name[256];
static const uint64_t sstable_id1 = 1048576;
static const uint64_t sstable_id2 = 1000000;
static const char* file_name1 = "./data/sst_1048576";
static const char* file_name2 = "./data/sst_1000000";
static FileInfoCache fic; 

class TestBlockCache : public ::testing::Test
{
public:
  static void SetUpTestCase()
  {
    char cmd[512];
    ObSSTableId ob_sstable_id;

    ob_sstable_id.sstable_file_id_ = sstable_id1;
    get_sstable_path(ob_sstable_id, file_name, 256);
    remove(file_name);
    sprintf(cmd, "mkdir -p %s; rm -rf %s; cp %s %s", 
            file_name, file_name, file_name1, file_name);
    system(cmd);

    ob_sstable_id.sstable_file_id_ = sstable_id2;
    get_sstable_path(ob_sstable_id, file_name, 256);
    remove(file_name);
    sprintf(cmd, "mkdir -p %s; rm -rf %s; cp %s %s", 
            file_name, file_name, file_name2, file_name);
    system(cmd);

    fic.init(1024);
  }

  static void TearDownTestCase()
  {
    fic.destroy();
  }

  virtual void SetUp()
  {

  }

  virtual void TearDown()
  {

  }
};

TEST_F(TestBlockCache, init)
{
  ObBlockCache bc(fic);
  ObBlockCacheConf conf;
  conf.block_cache_memsize_mb = 10;
  conf.ficache_max_num = 1024;
  ObBlockCacheConf tmp_conf;

  tmp_conf = conf;
  tmp_conf.block_cache_memsize_mb = 0;
  EXPECT_EQ(OB_ERROR, bc.init(tmp_conf));
  
  EXPECT_EQ(OB_SUCCESS, bc.init(conf));
  EXPECT_EQ(OB_SUCCESS, bc.init(conf));
  bc.destroy();
}

TEST_F(TestBlockCache, clear)
{
  ObBlockCache bc(fic);
  ObBlockCacheConf conf;
  conf.block_cache_memsize_mb = 10;
  conf.ficache_max_num = 1024;

  EXPECT_EQ(OB_ERROR, bc.clear());
  bc.init(conf);
  EXPECT_EQ(OB_SUCCESS, bc.clear());

  bc.init(conf);
  uint64_t sstable_ids[2] = {1048576, 1000000};
  int64_t offset = 0;
  int64_t nbyte = 2048;
  ObBufferHandle *buffer_handle = new ObBufferHandle();;
  EXPECT_EQ(nbyte, bc.get_block(sstable_ids[0], offset, nbyte, 
                                *buffer_handle, talbe_id));
  EXPECT_EQ(OB_ERROR, bc.clear());
  delete buffer_handle;
  EXPECT_EQ(OB_SUCCESS, bc.clear());
  bc.destroy();
}

TEST_F(TestBlockCache, get_block)
{
  ObBlockCache bc(fic);
  ObBlockCacheConf conf;
  conf.block_cache_memsize_mb = 10;
  conf.ficache_max_num = 1024;

  uint64_t sstable_ids[2] = {1048576, 1000000};
  int64_t offset = 0;
  int64_t nbyte = 2048;
  char *buffer = new char[nbyte];
  ObBufferHandle *buffer_handle = new ObBufferHandle();;
  ObBufferHandle tmp;
  EXPECT_EQ(-1, bc.get_block(sstable_ids[0], offset, nbyte,
                             *buffer_handle, talbe_id));

  EXPECT_EQ(OB_SUCCESS, bc.init(conf));
  EXPECT_EQ(nbyte, bc.get_block(sstable_ids[0], offset, nbyte, 
                                *buffer_handle, talbe_id));
  FILE *fd = fopen("./data/sst_1048576", "r");
  fread(buffer, 1, nbyte, fd);
  fclose(fd);
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));
  
  *buffer_handle = tmp;
  EXPECT_EQ(nbyte, bc.get_block(sstable_ids[0], offset, nbyte, 
                                *buffer_handle, talbe_id));
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));

  *buffer_handle = tmp;
  EXPECT_EQ(nbyte, bc.get_block(sstable_ids[1], offset, nbyte, 
                                *buffer_handle, talbe_id));
  fd = fopen("./data/sst_1000000", "r");
  fread(buffer, 1, nbyte, fd);
  fclose(fd);
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));
  
  *buffer_handle = tmp;
  EXPECT_EQ(nbyte, bc.get_block(sstable_ids[0], offset, nbyte, 
                                *buffer_handle, talbe_id));
  fd = fopen("./data/sst_1048576", "r");
  fread(buffer, 1, nbyte, fd);
  fclose(fd);
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));

  delete buffer_handle;
  delete buffer;
  bc.destroy();
}

TEST_F(TestBlockCache, test_get_block_aio)
{
  ObBlockCache bc(fic);
  ObBlockCacheConf conf;
  conf.block_cache_memsize_mb = 10;
  conf.ficache_max_num = 1024;

  uint64_t sstable_ids[2] = {1048576, 1000000};
  int64_t offset = 0;
  int64_t nbyte = 2048;
  int64_t timeo_us = 100000;
  char *buffer = new char[nbyte];
  ObBufferHandle *buffer_handle = new ObBufferHandle();
  ObBufferHandle tmp;
  EXPECT_EQ(-1, bc.get_block_aio(sstable_ids[0], offset, nbyte, 
                                 *buffer_handle, timeo_us, talbe_id, 0));

  bc.init(conf);
  EXPECT_EQ(-1, bc.get_block_aio(sstable_ids[0], offset, nbyte, 
                                 *buffer_handle, timeo_us, talbe_id, 0));

  ObBlockPositionInfos pos_infos;
  int64_t block_size = nbyte;
  pos_infos.block_count_ = 1;
  for (int64_t i = 0; i < 1; ++i)
  {
    pos_infos.position_info_[i].offset_ = i * block_size;
    pos_infos.position_info_[i].size_ = block_size;
  }

  EXPECT_EQ(OB_SUCCESS, bc.advise(sstable_ids[0], pos_infos, talbe_id, 0, false));
  EXPECT_EQ(nbyte, bc.get_block_aio(sstable_ids[0], offset, nbyte, 
                                    *buffer_handle, timeo_us, talbe_id, 0));
  FILE *fd = fopen("./data/sst_1048576", "r");
  fread(buffer, 1, nbyte, fd);
  fclose(fd);
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));
  
  *buffer_handle = tmp;
  EXPECT_EQ(OB_SUCCESS, bc.advise(sstable_ids[0], pos_infos, talbe_id, 0, false));
  EXPECT_EQ(nbyte, bc.get_block_aio(sstable_ids[0], offset, nbyte, 
                                    *buffer_handle, timeo_us, talbe_id, 0));
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));

  *buffer_handle = tmp;
  EXPECT_EQ(OB_SUCCESS, bc.advise(sstable_ids[1], pos_infos, talbe_id, 0, false));
  EXPECT_EQ(nbyte, bc.get_block_aio(sstable_ids[1], offset, nbyte, 
                                    *buffer_handle, timeo_us, talbe_id, 0));
  fd = fopen("./data/sst_1000000", "r");
  fread(buffer, 1, nbyte, fd);
  fclose(fd);
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));

  EXPECT_EQ(OB_SUCCESS, bc.advise(sstable_ids[0], pos_infos, talbe_id, 0, false));
  EXPECT_EQ(-1, bc.get_block_aio(sstable_ids[0], offset, nbyte, 
                                 *buffer_handle, 0, talbe_id, 0));
  
  *buffer_handle = tmp;
  EXPECT_EQ(OB_SUCCESS, bc.advise(sstable_ids[0], pos_infos, talbe_id, 0, true));
  EXPECT_EQ(nbyte, bc.get_block_aio(sstable_ids[0], offset, nbyte, 
                                *buffer_handle, timeo_us, talbe_id, 0));
  fd = fopen("./data/sst_1048576", "r");
  fread(buffer, 1, nbyte, fd);
  fclose(fd);
  EXPECT_EQ(0, memcmp(buffer, buffer_handle->get_buffer(), nbyte));

  *buffer_handle = tmp;
  EXPECT_EQ(OB_SUCCESS, bc.advise(sstable_ids[0], pos_infos, talbe_id, 0, true));
  EXPECT_EQ(nbyte, bc.get_block_aio(sstable_ids[0], offset, nbyte,
                                *buffer_handle, timeo_us, talbe_id, 0));

  delete buffer_handle;
  delete buffer;
  bc.destroy();
}

TEST_F(TestBlockCache, destroy)
{
  ObBlockCache bc(fic);
  ObBlockCacheConf conf;
  conf.block_cache_memsize_mb = 10;
  conf.ficache_max_num = 1024;

  EXPECT_EQ(OB_SUCCESS, bc.destroy());
  bc.init(conf);
  EXPECT_EQ(OB_SUCCESS, bc.destroy());

  bc.set_fileinfo_cache(fic);
  bc.init(conf);
  uint64_t sstable_ids[2] = {1048576, 1000000};
  int64_t offset = 0;
  int64_t nbyte = 2048;
  ObBufferHandle *buffer_handle = new ObBufferHandle();;
  EXPECT_EQ(nbyte, bc.get_block(sstable_ids[0], offset, nbyte, 
                                *buffer_handle, talbe_id));
  EXPECT_EQ(OB_ERROR, bc.destroy());
  delete buffer_handle;
  EXPECT_EQ(OB_SUCCESS, bc.destroy());
}

int main(int argc, char **argv)
{
  ob_init_memory_pool();
  TBSYS_LOGGER.setLogLevel("ERROR");
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
