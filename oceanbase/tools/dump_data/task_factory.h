#ifndef TASK_FACTORY_H_
#define TASK_FACTORY_H_

#include <set>
#include "common/ob_server.h"
#include "rpc_stub.h"
#include "task_manager.h"


namespace oceanbase
{
  namespace tools
  {
    class TaskFactory
    {
    public:
      TaskFactory();
      virtual ~TaskFactory();
      
    public:
      /// init status data
      int init(const int64_t version, const int64_t timeout, const common::ObServer & root_server, 
        const common::ObSchemaManagerV2 * schema, RpcStub * rpc, TaskManager * manager);
      
      /// get all the dump task
      int get_all_tablets(uint64_t & tablet_count);
      
      // add table
      int add_table(const char * table_name);
      
      /// check string length and ptr data
      static bool check_string(const common::ObString & name);

      /// check is max rowkey
      static bool is_max_rowkey(const int64_t max_len, const char rowkey[], const int64_t key_len);
    
    private:
      /// check inner stat
      bool check_inner_stat(void) const;
      
      /// init scan param
      int init_scan_param(const uint64_t table_id, uint64_t & max_len, common::ObScanParam & param);
      
      /// init new task
      int init_new_task (const common::ObString & table_name, const common::ObString & start_key,
          const common::ObString & end_key, common::ObScanParam & scan_param, TaskInfo & task) const;
      
      /// insert the new task
      int insert_new_task(const TabletLocation & list, TaskInfo & task);

      /// get table max rowkey lenght
      int get_max_len(const uint64_t table_id, uint64_t & max_len);
      
      /// add all table columns
      int add_all_columns(const uint64_t table_id, common::ObScanParam & param);
      
      /// get table tablets
      int get_table_tablet(const char * table_name, const uint64_t table_id, uint64_t & count);
    
    private:
      RpcStub * rpc_;
      int64_t timeout_;
      int64_t memtable_version_;
      common::ObServer root_server_;
      std::set<std::string> dump_tables_;
      const common::ObSchemaManagerV2 * schema_;
      TaskManager * task_manager_;
    };

    inline bool TaskFactory::check_inner_stat(void) const
    {
      return ((rpc_ != NULL) && (schema_ != NULL) && (task_manager_ != NULL));
    }
  }
}


#endif //TASK_FACTORY_H_
