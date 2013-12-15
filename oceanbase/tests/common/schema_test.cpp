#include <gtest/gtest.h>
#include "common/ob_schema.h"
#include "common/ob_malloc.h"
#include "common/file_directory_utils.h"
using namespace oceanbase;
using namespace oceanbase::common;
TEST(SchemaTest, basicTest)
{
  tbsys::CConfig c1;
  ObSchemaManager *schema_manager = new ObSchemaManager();
  ASSERT_EQ(true, schema_manager->parse_from_file("./test1.ini", c1));

  schema_manager->print_info();
  ASSERT_EQ(1001, schema_manager->get_table_id("u_collect_item_id"));
  ASSERT_EQ(1003, schema_manager->get_table_id("collect_item_id"));

  ASSERT_EQ(0, strcmp("u_collect_item_id",schema_manager->get_table_name(1001)));
  ASSERT_EQ(0, strcmp("collect_item_id",schema_manager->get_table_name(1003)));

  const ObSchema* table = schema_manager->get_table_schema(1003);
  ASSERT_EQ(true, table != NULL);
  ASSERT_EQ(1003, table->get_table_id());
  ASSERT_EQ(3, schema_manager->get_column_id(1003,"new_price"));
  ASSERT_EQ(0, strcmp("new_price", schema_manager->get_column_name(1003, 3)));
  ASSERT_EQ(72, table->get_block_size());
  ASSERT_EQ(true, table->is_use_bloomfilter());
  ASSERT_EQ(0, strcmp("lalala", table->get_compress_func_name()));
  const ObColumnSchema* column = schema_manager->get_column_schema(1003, 3);
  ASSERT_EQ(true, column != NULL);
  ASSERT_EQ(3, column->get_id());
  delete schema_manager;
}


TEST(SchemaTestV2, basicTest)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  ASSERT_EQ(true, schema_manager->parse_from_file("./test1.ini", c1));

  ASSERT_EQ(6,schema_manager->get_column_count());
  ASSERT_EQ(3,schema_manager->get_table_count());
  schema_manager->print_info();

  const ObColumnSchemaV2 *col = schema_manager->column_begin();
  for(;col != schema_manager->column_end(); ++col)
  {
    col->print_info();
  }

  int32_t size = 0;
  col = schema_manager->get_table_schema(1001,size);
  ASSERT_EQ(2,size);

  int32_t idx = 0;
  col = schema_manager->get_column_schema(1002,2,&idx);
  ASSERT_EQ(0,idx);
  ASSERT_TRUE(NULL != col);

  col = schema_manager->get_column_schema(1002,3,&idx);
  ASSERT_EQ(1,idx);
  ASSERT_TRUE(NULL != col);
  
  col = schema_manager->get_column_schema("collect_info","item_name");
  ASSERT_EQ(col->get_id(),2UL);

  const ObTableSchema *table = schema_manager->get_table_schema("collect_info");
  ASSERT_EQ(table->get_table_id(),1002);

  ObString table_name_str;
  table_name_str.assign_ptr("collect_info",12);
  table = schema_manager->get_table_schema(table_name_str);
  ASSERT_EQ(table->get_table_id(),1002);

  delete schema_manager;
}


#if 1
//TEST(SchemaTest, serialize)
//{
//  tbsys::CConfig c1;
//  ObSchemaManager *schema_manager = new ObSchemaManager(1);;
//  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));
//
//  int64_t len = schema_manager->get_serialize_size();
//  char *buf =(char *)malloc(len);
//  assert(buf != NULL);
//  int64_t pos = 0;
//  ASSERT_EQ(0,schema_manager->serialize(buf,len,pos));
//
//  ASSERT_EQ(pos,len);
//
//  pos = 0;
//  ObSchemaManager *schema_manager_1 = new ObSchemaManager(1);
//  ASSERT_EQ(0,schema_manager_1->deserialize(buf,len,pos));
//  ASSERT_EQ(pos,len);
//
//  ASSERT_EQ(schema_manager_1->get_serialize_size(),len);
//  char *buf_2 = (char *)malloc(len);
//  assert(buf_2 != NULL);
//
//  pos = 0;
//  schema_manager_1->serialize(buf_2,len,pos);
//  ASSERT_EQ(pos,len);
//
//  ASSERT_EQ(0,memcmp(buf,buf_2,len));
//
//
//  delete schema_manager;
//  delete schema_manager_1;
//  free(buf);
//  buf = NULL;
//  free(buf_2);
//  buf_2 = NULL;
//}

TEST(SchemaTestV2, serialize)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));

  ASSERT_EQ(6,schema_manager->get_create_time_column_id(1001));
  ASSERT_EQ(7,schema_manager->get_modify_time_column_id(1001));

  int64_t len = schema_manager->get_serialize_size();
  char *buf =(char *)malloc(len);
  assert(buf != NULL);
  int64_t pos = 0;
  ASSERT_EQ(0,schema_manager->serialize(buf,len,pos));

  ASSERT_EQ(pos,len);

  pos = 0;
  ObSchemaManagerV2 *schema_manager_1 = new ObSchemaManagerV2();
  ASSERT_EQ(0,schema_manager_1->deserialize(buf,len,pos));
  ASSERT_EQ(pos,len);

  ASSERT_EQ(schema_manager_1->get_serialize_size(),len);
  char *buf_2 = (char *)malloc(len);
  assert(buf_2 != NULL);

  ASSERT_EQ(6,schema_manager_1->get_create_time_column_id(1001));
  ASSERT_EQ(7,schema_manager_1->get_modify_time_column_id(1001));
  
  pos = 0;
  schema_manager_1->serialize(buf_2,len,pos);
  ASSERT_EQ(pos,len);

  ASSERT_EQ(0,memcmp(buf,buf_2,len));


  delete schema_manager;
  delete schema_manager_1;
  free(buf);
  buf = NULL;
  free(buf_2);
  buf_2 = NULL;
}

TEST(SchemaTestV2,column_group)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));

  int32_t size = OB_MAX_COLUMN_GROUP_NUMBER;
  uint64_t group[OB_MAX_COLUMN_GROUP_NUMBER];
  ASSERT_EQ(0,schema_manager->get_column_groups(1001,group,size));

  ASSERT_EQ(3,size);
  ASSERT_EQ(11,group[0]);
  ASSERT_EQ(12,group[1]);

  delete schema_manager;
}

TEST(SchemaTestV2,column_group_all)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));
  uint64_t column_id = 0;
  int64_t duration = 0;
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(3, column_id);
  ASSERT_EQ(7, duration);
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1002)->get_expire_condition(column_id, duration));
  ASSERT_EQ(4, column_id);
  ASSERT_EQ(10, duration);
  int32_t size = OB_MAX_COLUMN_GROUP_NUMBER;
  uint64_t group[OB_MAX_COLUMN_GROUP_NUMBER];
  ASSERT_EQ(0,schema_manager->get_column_groups(1001,group,size));

  ASSERT_EQ(3,size);
  ASSERT_EQ(11,group[0]);
  ASSERT_EQ(12,group[1]);

  const ObColumnSchemaV2 *col = schema_manager->get_table_schema(1001,size);
  ASSERT_EQ(31,size);

  int32_t index[OB_MAX_COLUMN_GROUP_NUMBER];
  size = OB_MAX_COLUMN_GROUP_NUMBER;
  int ret= schema_manager->get_column_index("collect_info","info_user_nick",index,size);
  ASSERT_EQ(0,ret);
  ASSERT_EQ(3,size);

  for(int32_t i=0;i<size;++i)
  {
    ASSERT_EQ(2,schema_manager->get_column_schema(index[i])->get_id());
  }

  ObColumnSchemaV2* columns[OB_MAX_COLUMN_GROUP_NUMBER];
  size = OB_MAX_COLUMN_GROUP_NUMBER;

  ret = schema_manager->get_column_schema(1001,5,columns,size);
  ASSERT_EQ(0,ret);
  ASSERT_EQ(2,size);

  for(int32_t i=0;i<size;++i)
  {
    ASSERT_EQ(5,columns[i]->get_id());
  }

  //drop column group
  int64_t len = schema_manager->get_serialize_size();
  char *buf =(char *)malloc(len);
  assert(buf != NULL);
  int64_t pos = 0;
  ASSERT_EQ(0,schema_manager->serialize(buf,len,pos));
  ASSERT_EQ(pos,len);

  ObSchemaManagerV2 *schema_manager_1 = new ObSchemaManagerV2();
  schema_manager_1->set_drop_column_group();
  pos = 0;
  ASSERT_EQ(0,schema_manager_1->deserialize(buf,len,pos));
  ASSERT_EQ(pos,len);
  ASSERT_EQ(42,schema_manager_1->get_column_count());

  schema_manager_1->print_info();

  delete schema_manager;
}



TEST(SchemaTestV2,equal)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));

  uint64_t column_id = 0;
  int64_t duration = 0;
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(3, column_id);
  ASSERT_EQ(7, duration);
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1002)->get_expire_condition(column_id, duration));
  ASSERT_EQ(4, column_id);
  ASSERT_EQ(10, duration);

  ObSchemaManagerV2 *schema_manager_1 = new ObSchemaManagerV2();
  *schema_manager_1 = *schema_manager;

  int64_t len = schema_manager->get_serialize_size();
  char *buf =(char *)malloc(len);
  assert(buf != NULL);
  int64_t pos = 0;
  ASSERT_EQ(0,schema_manager->serialize(buf,len,pos));
  ASSERT_EQ(pos,len);

  ASSERT_EQ(schema_manager_1->get_serialize_size(),len);

  char *buf_2 = (char *)malloc(len);
  assert(buf_2 != NULL);

  pos = 0;
  ASSERT_EQ(0,schema_manager_1->serialize(buf_2,len,pos));
  ASSERT_EQ(pos,len);

  ASSERT_EQ(0,memcmp(buf,buf_2,len));

  delete buf;
  delete buf_2;
  delete schema_manager;
  delete schema_manager_1;
}
#endif

TEST(SchemaTestV2,schema_1_to_2)
{
  tbsys::CConfig c1;
  ObSchemaManager *schema_manager = new ObSchemaManager(1);
  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));

  int64_t len = schema_manager->get_serialize_size();
  char *buf =(char *)malloc(len);
  assert(buf != NULL);
  int64_t pos = 0;
  ASSERT_EQ(0,schema_manager->serialize(buf,len,pos));
  //ASSERT_EQ(pos,len);

  ObSchemaManagerV2 *schema_manager_1 = new ObSchemaManagerV2(1);

  pos = 0;
  ASSERT_EQ(0,schema_manager_1->deserialize(buf,len,pos));

  schema_manager_1->print_info();
  //ASSERT_EQ(0,schema_1_to_2(*schema_manager,*schema_manager_1));

  //len = schema_manager_1->get_serialize_size();
  
  //char *buf_2 = (char *)malloc(len);
  //assert(buf_2 != NULL);

  //pos = 0;
  //ASSERT_EQ(0,schema_manager_1->serialize(buf_2,len,pos));
  //ASSERT_EQ(pos,len);

  //ASSERT_EQ(0,memcmp(buf,buf_2,len));

  delete buf;
  //delete buf_2;
  delete schema_manager;
  delete schema_manager_1;
}

//TEST(SchemaTest,equal)
//{
//  tbsys::CConfig c1;
//  ObSchemaManager *schema_manager = new ObSchemaManager(1);;
//  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));
//
//  ObSchemaManager *schema_manager_1 = new ObSchemaManager(1);
//  *schema_manager_1 = *schema_manager;
//
//
//  int64_t len = schema_manager->get_serialize_size();
//  char *buf =(char *)malloc(len);
//  assert(buf != NULL);
//  int64_t pos = 0;
//  ASSERT_EQ(0,schema_manager->serialize(buf,len,pos));
//  ASSERT_EQ(pos,len);
//
//  ASSERT_EQ(schema_manager_1->get_serialize_size(),len);
//
//  char *buf_2 = (char *)malloc(len);
//  assert(buf_2 != NULL);
//
//  pos = 0;
//  ASSERT_EQ(0,schema_manager_1->serialize(buf_2,len,pos));
//  ASSERT_EQ(pos,len);
//
//  ASSERT_EQ(0,memcmp(buf,buf_2,len));
//
//  delete buf;
//  delete buf_2;
//  delete schema_manager;
//  delete schema_manager_1;
//}
#if 1
TEST(SchemaTest, __compitable)
{
  tbsys::CConfig c1;
  ObSchemaManager *schema_manager = new ObSchemaManager(1);;
  ASSERT_EQ(true, schema_manager->parse_from_file("./test1.ini", c1));

  fprintf(stderr,"%lu",sizeof(schema_manager));
  
  tbsys::CConfig c2;
  ObSchemaManager *schema_manager_c1 = new ObSchemaManager(1);
  ASSERT_EQ(true, schema_manager_c1->parse_from_file("./comptiable1.ini", c2));

  tbsys::CConfig c3;
  ObSchemaManager *schema_manager_c2 = new ObSchemaManager(1);
  ASSERT_EQ(true, schema_manager_c2->parse_from_file("./comptiable2.ini", c3));

  ASSERT_EQ(true,schema_manager->is_compatible(*schema_manager_c1));
  ASSERT_EQ(true,schema_manager->is_compatible(*schema_manager_c2));
  delete schema_manager;
  delete schema_manager_c1;
  delete schema_manager_c2;
}
#endif
TEST(SchemaTestV2, btefff)
{
  tbsys::CConfig *c1 = new tbsys::CConfig();
  //tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2(1);
  TBSYS_LOG(DEBUG,"c1:%lu,schema_manager:%lu",sizeof(*c1),sizeof(*schema_manager));
  TBSYS_LOG(DEBUG,"c1:%p,schema_manager:%p",c1,schema_manager);
  ASSERT_EQ(true, schema_manager->parse_from_file("./collect_schema.ini", *c1));
  TBSYS_LOG(DEBUG,"c1:%lu,schema_manager:%lu",sizeof(*c1),sizeof(*schema_manager));
  TBSYS_LOG(DEBUG,"c1:%p,schema_manager:%p",c1,schema_manager);

  //schema_manager->print_info();

  delete schema_manager;

  TBSYS_LOG(DEBUG,"end--");
  delete c1;
}
TEST(SchemaTest, SchemaV2notorder)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 schema_manager(1);
  ASSERT_EQ(true, schema_manager.parse_from_file("./schema.ini", c1));
  schema_manager.print_info();
  ASSERT_EQ(2, schema_manager.table_end() - schema_manager.table_begin());
  const ObTableSchema *table_schema = schema_manager.table_begin();
  ASSERT_EQ((uint64_t)1001, table_schema->get_table_id());
  ASSERT_EQ(1, table_schema->get_table_type());
  ASSERT_EQ(0, strcmp("collect_info", table_schema->get_table_name()));
  ASSERT_EQ((uint64_t)22, table_schema->get_max_column_id());
  ++table_schema;
  ASSERT_EQ((uint64_t)1002, table_schema->get_table_id());
  ASSERT_EQ(1, table_schema->get_table_type());
  ASSERT_EQ(0, strcmp("item_info", table_schema->get_table_name()));
  ASSERT_EQ((uint64_t)12, table_schema->get_max_column_id());
    
  int32_t table_size = 0; 
  const ObTableSchema* table = schema_manager.get_table_schema((uint64_t)1001);
  ASSERT_EQ(true, table != NULL);
  ASSERT_EQ((uint64_t)1001, table->get_table_id());
  ASSERT_EQ(80, table->get_block_size());
  ASSERT_EQ(true, table->is_use_bloomfilter());
  ASSERT_EQ(0, strcmp("lalala", table->get_compress_func_name()));
  
  const ObColumnSchemaV2* table_schema_column = schema_manager.get_table_schema((uint64_t)1001, table_size);
  ASSERT_EQ(true, table_schema_column != NULL);
  ASSERT_EQ(23, table_size);
  

  int32_t group_size = OB_MAX_COLUMN_GROUP_NUMBER;
  uint64_t column_groups[OB_MAX_COLUMN_GROUP_NUMBER];
  
  int ret = schema_manager.get_column_groups((uint64_t)1001, column_groups, group_size);
  ret++;
  ASSERT_EQ((uint64_t)0, column_groups[0]);
  ASSERT_EQ(5, group_size);
  for (int64_t index = 1; index < group_size; ++index)
  {
    ASSERT_EQ((uint64_t)10+index, column_groups[index]);
  }
  
  const ObColumnSchemaV2* default_group_schema = schema_manager.get_group_schema((uint64_t)1001,(uint64_t) 0, group_size);
  ASSERT_EQ(2, group_size);
  ASSERT_EQ(0, strcmp("user_nick", default_group_schema->get_name()));
  ASSERT_EQ((uint64_t)2, default_group_schema->get_id());
  ASSERT_EQ((uint64_t)1001, default_group_schema->get_table_id());
 
  const ObColumnSchemaV2* first_group_schema = schema_manager.get_group_schema(1001, 11, group_size);
  ASSERT_EQ(3, group_size);
  ASSERT_EQ(0, strcmp("note", first_group_schema->get_name()));
  ASSERT_EQ((uint64_t)4, first_group_schema->get_id());

  const ObColumnSchemaV2* column = schema_manager.get_column_schema((uint64_t)1001, (uint64_t)11);
  ASSERT_EQ(true, column != NULL);
  ASSERT_EQ(11, column->get_id());
  ASSERT_EQ(0, strcmp("title", column->get_name()));
  ASSERT_EQ((uint64_t)12, column->get_column_group_id());
  int32_t index_array[OB_MAX_COLUMN_GROUP_NUMBER];
  int32_t index_size = OB_MAX_COLUMN_GROUP_NUMBER;
  ret = schema_manager.get_column_index((uint64_t)1001, (uint64_t)11, index_array, index_size);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(2, index_size);
  ASSERT_EQ(7, index_array[0]);
  ASSERT_EQ(18,index_array[1]);
}

TEST(SchemaTest, checkExpire)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema_error_expire.ini",c1));

  uint64_t column_id = 0;
  int64_t duration = 0;
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager->get_table_schema(1002)->get_expire_condition(column_id, duration));
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1003)->get_expire_condition(column_id, duration));
  ASSERT_EQ(3, column_id);
  ASSERT_EQ(0, duration);
  
  ASSERT_EQ(6,schema_manager->get_create_time_column_id(1001));
  ASSERT_EQ(7,schema_manager->get_modify_time_column_id(1001));
}

TEST(SchemaTest, expireInfo)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  ASSERT_EQ(true,schema_manager->parse_from_file("./collect_schema.ini",c1));

  uint64_t column_id = 0;
  int64_t duration = 0;
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(3, column_id);
  ASSERT_EQ(7, duration);
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1002)->get_expire_condition(column_id, duration));
  ASSERT_EQ(4, column_id);
  ASSERT_EQ(10, duration);

  ASSERT_EQ(6,schema_manager->get_create_time_column_id(1001));
  ASSERT_EQ(7,schema_manager->get_modify_time_column_id(1001));

  int64_t len = schema_manager->get_serialize_size();
  char *buf =(char *)malloc(len);
  assert(buf != NULL);
  int64_t pos = 0;
  ASSERT_EQ(0,schema_manager->serialize(buf,len,pos));

  ASSERT_EQ(pos,len);

  pos = 0;
  ObSchemaManagerV2 *schema_manager_1 = new ObSchemaManagerV2();
  ASSERT_EQ(0,schema_manager_1->deserialize(buf,len,pos));
  ASSERT_EQ(pos,len);
  
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(3, column_id);
  ASSERT_EQ(7, duration);
  ASSERT_EQ(OB_SUCCESS, schema_manager->get_table_schema(1002)->get_expire_condition(column_id, duration));
  ASSERT_EQ(4, column_id);
  ASSERT_EQ(10, duration);

  ASSERT_EQ(schema_manager_1->get_serialize_size(),len);
  char *buf_2 = (char *)malloc(len);
  assert(buf_2 != NULL);

  ASSERT_EQ(6,schema_manager_1->get_create_time_column_id(1001));
  ASSERT_EQ(7,schema_manager_1->get_modify_time_column_id(1001));
  
  pos = 0;
  schema_manager_1->serialize(buf_2,len,pos);
  ASSERT_EQ(pos,len);

  ASSERT_EQ(0,memcmp(buf,buf_2,len));


  delete schema_manager;
  delete schema_manager_1;
  free(buf);
  buf = NULL;
  free(buf_2);
  buf_2 = NULL;

  tbsys::CConfig c2;
  ObSchemaManagerV2 *schema_manager2 = new ObSchemaManagerV2();
  ASSERT_EQ(true,schema_manager2->parse_from_file("./schema.ini",c2));
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager2->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager2->get_table_schema(1002)->get_expire_condition(column_id, duration));

  len = schema_manager2->get_serialize_size();
  buf =(char *)malloc(len);
  assert(buf != NULL);
  pos = 0;
  ASSERT_EQ(0,schema_manager2->serialize(buf,len,pos));

  ASSERT_EQ(pos,len);

  pos = 0;
  ObSchemaManagerV2 *schema_manager3 = new ObSchemaManagerV2();
  ASSERT_EQ(0,schema_manager3->deserialize(buf,len,pos));
  ASSERT_EQ(pos,len);
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager3->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager3->get_table_schema(1002)->get_expire_condition(column_id, duration));
  delete schema_manager2;
  delete schema_manager3;
  free(buf);
  buf = NULL;
}

TEST(SchemaTest, compatible_with_v020)
{
  tbsys::CConfig c1;
  ObSchemaManagerV2 *schema_manager = new ObSchemaManagerV2();
  FileDirectoryUtils fu;
  int64_t size = fu.get_size("schemamanagerbuffer020");
  char *buf =(char *)malloc(size);
  assert(buf != NULL);
  int fd = open("schemamanagerbuffer020", O_RDONLY);
  read(fd, buf, size);
  close(fd);
  int64_t pos = 0;
  ASSERT_EQ(0,schema_manager->deserialize(buf,size,pos));
  ASSERT_EQ(pos,size);

  uint64_t column_id = 0;
  int64_t duration = 0;
  ASSERT_EQ(6,schema_manager->get_create_time_column_id(1001));
  ASSERT_EQ(7,schema_manager->get_modify_time_column_id(1001));  
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager->get_table_schema(1001)->get_expire_condition(column_id, duration));
  ASSERT_EQ(OB_ENTRY_NOT_EXIST, schema_manager->get_table_schema(1002)->get_expire_condition(column_id, duration));
  free(buf);
  buf = NULL;
}


int main(int argc, char **argv)
{
  ob_init_memory_pool();
  ::testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
