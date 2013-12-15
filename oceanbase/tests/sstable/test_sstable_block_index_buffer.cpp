/*
 * (C) 2007-2010 Taobao Inc. 
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * test_sstable_block_index_buffer.cpp is for what ...
 *
 * Version: ***: test_sstable_block_index_buffer.cpp  Wed Mar 23 14:25:51 2011 fangji.hcm Exp $
 *
 * Authors:
 *   Author fangji.hcm <fangji.hcm@taobao.com>
 *     -some work detail if you want 
 *
 */

#include <tblog.h>
#include <gtest/gtest.h>
#include "sstable/ob_sstable_schema.h"
#include "common/ob_object.h"
#include "sstable/ob_sstable_row.h"
#include "sstable/ob_sstable_block_index_buffer.h"
#include "key.h"

using namespace oceanbase::common;
using namespace oceanbase::sstable;

namespace oceanbase
{
  namespace tests
  {
    namespace sstable 
    {
      class TestObSSTableBlockIndexBuffer: public ::testing::Test
      {
      public:
        virtual void SetUp()
        {
            
        }
        
        virtual void TearDown()
        {
            
        }
      };
      
      TEST_F(TestObSSTableBlockIndexBuffer, test_init)
      {
        ObSSTableBlockIndexBuffer index_buffer;

        EXPECT_EQ(0, index_buffer.get_data_size());
        EXPECT_EQ(0, index_buffer.get_total_size());
        EXPECT_TRUE(NULL == index_buffer.get_block_head());
        EXPECT_TRUE(NULL == index_buffer.get_block_tail());
      }

      TEST_F(TestObSSTableBlockIndexBuffer, test_add_key)
      {
        int ret = OB_SUCCESS;
        ObSSTableBlockIndexBuffer index_block;
                
        Key tmp_key(1000, 0, 100);
        ObString key(tmp_key.key_len(), tmp_key.key_len(), tmp_key.get_ptr());
        ret = index_block.add_key(key);

        EXPECT_TRUE(OB_SUCCESS == ret);
        EXPECT_TRUE(index_block.get_data_size() > 0);
        EXPECT_EQ(17, index_block.get_data_size());
        EXPECT_TRUE(index_block.get_total_size() > 0);
        EXPECT_TRUE(index_block.get_data_size() < index_block.get_total_size());
        EXPECT_TRUE(NULL != index_block.get_block_head());
        EXPECT_TRUE(NULL != index_block.get_block_tail());
        EXPECT_TRUE(index_block.get_block_tail() == index_block.get_block_head());
        
      }

      TEST_F(TestObSSTableBlockIndexBuffer, test_add_many_key)
      {
        ObSSTableBlockIndexBuffer index_buffer;
        int ret = OB_SUCCESS;
        
        //need 2 mem block to store all(200,000) key
        for (int i = 0; i < 200000; ++i)
        {
          Key tmp_key(i, i%128, i);
          ObString key(tmp_key.key_len(), tmp_key.key_len(), tmp_key.get_ptr());
          ret = index_buffer.add_key(key);
          EXPECT_TRUE(OB_SUCCESS == ret);
        }

        EXPECT_TRUE(index_buffer.get_data_size() > 0);
        EXPECT_TRUE(index_buffer.get_total_size() > 0);
        EXPECT_EQ(ObSSTableBlockIndexBuffer::DEFAULT_MEM_BLOCK_SIZE*2, index_buffer.get_total_size());
        int64_t real_size = 0;
        const ObSSTableBlockIndexBuffer::MemBlock* tmp = index_buffer.get_block_head();
        while(NULL != tmp)
        {
          real_size += tmp->cur_pos_;
          tmp = tmp->next_;
        }
        EXPECT_EQ(real_size, index_buffer.get_data_size());
        EXPECT_TRUE(NULL != index_buffer.get_block_head());
        EXPECT_TRUE(NULL != index_buffer.get_block_tail());
        EXPECT_TRUE(index_buffer.get_block_tail() == index_buffer.get_block_head()->next_);
        
      }

      TEST_F(TestObSSTableBlockIndexBuffer, test_add_index_item)
      {
        ObSSTableBlockIndexBuffer index_buffer;
        int ret = OB_SUCCESS;

        ObSSTableBlockIndexItem index_item;
        index_item.reserved16_ = 0;
        index_item.column_group_id_ = 1;
        index_item.table_id_ = 1000;
        index_item.block_record_size_ = 100;
        index_item.block_end_key_size_ = 200;
        index_item.reserved_ = 0;

        ret = index_buffer.add_index_item(index_item);
        EXPECT_TRUE(OB_SUCCESS == ret);
        int64_t data_size = index_item.get_serialize_size();
        EXPECT_EQ(16, data_size);
        EXPECT_EQ(data_size, index_buffer.get_data_size());
        //EXPECT_EQ(ObSSTableBlockIndexBuffer::DEFAULT_MEM_BLOCK_SIZE, index_buffer.get_total_size());
        EXPECT_TRUE(index_buffer.get_data_size() < index_buffer.get_total_size());
        EXPECT_TRUE(NULL != index_buffer.get_block_head());
        EXPECT_TRUE(NULL != index_buffer.get_block_tail());
        EXPECT_TRUE(index_buffer.get_block_tail() == index_buffer.get_block_head());
        
      }

      TEST_F(TestObSSTableBlockIndexBuffer, test_add_many_index_item)
      {
        ObSSTableBlockIndexBuffer index_buffer;
        int ret = OB_SUCCESS;
        
        //200,000 index item need 2 mem block to store
        for (int i=0; i<200000; ++i)
        {
          ObSSTableBlockIndexItem index_item;
          index_item.reserved16_ = 0;
          index_item.column_group_id_ = 1;
          index_item.table_id_ = 1000;
          index_item.block_record_size_ = 100;
          index_item.block_end_key_size_ = 200;
          index_item.reserved_ = 0;
          
          ret = index_buffer.add_index_item(index_item);
          EXPECT_TRUE(OB_SUCCESS == ret);
        }
        
        EXPECT_EQ(16*200000, index_buffer.get_data_size());
        EXPECT_EQ(ObSSTableBlockIndexBuffer::DEFAULT_MEM_BLOCK_SIZE*2, index_buffer.get_total_size());
        EXPECT_TRUE(index_buffer.get_data_size() < index_buffer.get_total_size());
        EXPECT_TRUE(NULL != index_buffer.get_block_head());
        EXPECT_TRUE(NULL != index_buffer.get_block_tail());
        EXPECT_TRUE(index_buffer.get_block_tail() == index_buffer.get_block_head()->next_);
        
      }

      TEST_F(TestObSSTableBlockIndexBuffer, test_clear)
      {
        ObSSTableBlockIndexBuffer index_buffer;
        int ret = OB_SUCCESS;
        
        //200,000 index item need 2 mem block to store
        for (int i=0; i<200000; ++i)
        {
          ObSSTableBlockIndexItem index_item;
          index_item.reserved16_ = 0;
          index_item.column_group_id_ = 1;
          index_item.table_id_ = 1000;
          index_item.block_record_size_ = 100;
          index_item.block_end_key_size_ = 200;
          index_item.reserved_ = 0;
          
          ret = index_buffer.add_index_item(index_item);
          EXPECT_TRUE(OB_SUCCESS == ret);
        }
        
        EXPECT_EQ(16*200000, index_buffer.get_data_size());
        EXPECT_EQ(ObSSTableBlockIndexBuffer::DEFAULT_MEM_BLOCK_SIZE*2, index_buffer.get_total_size());
        EXPECT_TRUE(index_buffer.get_data_size() < index_buffer.get_total_size());
        EXPECT_TRUE(NULL != index_buffer.get_block_head());
        EXPECT_TRUE(NULL != index_buffer.get_block_tail());
        EXPECT_TRUE(index_buffer.get_block_tail() == index_buffer.get_block_head()->next_);
                
        index_buffer.clear();
        EXPECT_EQ(0, index_buffer.get_data_size());
        EXPECT_EQ(0, index_buffer.get_total_size());
        EXPECT_TRUE(NULL == index_buffer.get_block_head());
        EXPECT_TRUE(NULL == index_buffer.get_block_tail());
      }

      TEST_F(TestObSSTableBlockIndexBuffer, test_reset)
      {
        ObSSTableBlockIndexBuffer index_buffer;
        int ret = OB_SUCCESS;
        
        //200,000 index item need 2 mem block to store
        for (int i=0; i<200000; ++i)
        {
          ObSSTableBlockIndexItem index_item;
          index_item.reserved16_ = 0;
          index_item.column_group_id_ = 1;
          index_item.table_id_ = 1000;
          index_item.block_record_size_ = 100;
          index_item.block_end_key_size_ = 200;
          index_item.reserved_ = 0;
          
          ret = index_buffer.add_index_item(index_item);
          EXPECT_TRUE(OB_SUCCESS == ret);
        }
        const ObSSTableBlockIndexBuffer::MemBlock* old_head = index_buffer.get_block_head();
        EXPECT_EQ(16*200000, index_buffer.get_data_size());
        EXPECT_EQ(ObSSTableBlockIndexBuffer::DEFAULT_MEM_BLOCK_SIZE*2, index_buffer.get_total_size());
        EXPECT_TRUE(index_buffer.get_data_size() < index_buffer.get_total_size());
        EXPECT_TRUE(NULL != index_buffer.get_block_head());
        EXPECT_TRUE(NULL != index_buffer.get_block_tail());
        EXPECT_TRUE(index_buffer.get_block_tail() == index_buffer.get_block_head()->next_);
                
        index_buffer.reset();
        //EXPECT_EQ(ObSSTableBlockIndexBuffer::DEFAULT_MEM_BLOCK_SIZE, index_buffer.get_data_size());
        EXPECT_EQ(ObSSTableBlockIndexBuffer::DEFAULT_MEM_BLOCK_SIZE, index_buffer.get_total_size());
        EXPECT_TRUE(old_head == index_buffer.get_block_head());
        EXPECT_TRUE(old_head == index_buffer.get_block_tail());
      }

    }//namespace sstable
  }//namespace test
}//namespace oceanbase

int main(int argc, char** argv)
{
  ob_init_memory_pool();
  TBSYS_LOGGER.setLogLevel("ERROR");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
