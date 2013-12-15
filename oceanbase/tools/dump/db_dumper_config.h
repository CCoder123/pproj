/*
 * =====================================================================================
 *
 *       Filename:  db_dump_config.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/14/2011 07:44:04 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yushun.swh (DBA Group), yushun.swh@taobao.com
 *        Company:  taobao
 *
 * =====================================================================================
 */

#ifndef  OB_API_DB_SCHEMA_INC
#define  OB_API_DB_SCHEMA_INC
#include <string>
#include <vector>
#include <map>
#include "db_record_filter.h"

#define DUMP_CONFIG (DbDumpConfigMgr::get_instance())

class DbTableConfig {
  public:
    struct RowkeyItem {
      std::string name;
      int64_t start_pos;
      int64_t end_pos;
      oceanbase::common::ObObjType type;
      int endian;

      bool operator==(const RowkeyItem &item) { return name == item.name; }
    };

  public:
    DbTableConfig(std::string tab, std::vector<const char *> &columns) { 
      table_ = tab;
      filter_ = NULL;

      std::vector<const char *>::iterator itr = columns.begin();
      while(itr != columns.end()) {
        columns_.push_back(std::string(*itr));
        itr++;
      }
    }

    DbTableConfig() {
      table_id_ = 0;
      filter_ = NULL;
    }

    ~DbTableConfig() {
    }

    DbTableConfig(const DbTableConfig &rhs)
    {
      table_id_ = rhs.table_id_;
      table_= rhs.table_;
      columns_ = rhs.columns_;
      filter_ = rhs.filter_;
      rowkey_columns_ = rhs.rowkey_columns_;
      output_columns_ = rhs.output_columns_;
      revise_columns_ = rhs.revise_columns_;
    }

    const std::string &get_table() { return table_; }

    inline std::vector<std::string>& get_columns() { 
      return columns_; 
    }

    std::vector<RowkeyItem>& rowkey_columns() { return rowkey_columns_; }
    std::vector<std::string>& output_columns() { return output_columns_; }

    std::string &table() { return table_; }
    uint64_t table_id() { return table_id_; }

    uint64_t table_id_;
    DbRowFilter *filter_;

    void parse_rowkey_item(std::vector<const char *> &vec);
    void parse_output_columns(std::string out_columns);

    std::vector<const char *> & get_revise_columns() { return revise_columns_; }

    void set_revise_columns(std::vector<const char *> &revise_columns) {
      revise_columns_ = revise_columns;
    }

    bool is_revise_column(std::string &column);

  private:
    std::string table_;
    std::vector<std::string> columns_;
    std::vector<RowkeyItem> rowkey_columns_;
    std::vector<std::string> output_columns_;
  
    std::vector<const char *> revise_columns_;
};

class DbDumpConfigMgr {
  public:
    ~DbDumpConfigMgr();

    int load(const char *file);
    static DbDumpConfigMgr *get_instance(); 
    std::vector<DbTableConfig>& get_configs() { return configs_; }

    int get_table_config(std::string table, DbTableConfig* &cfg);
    int get_table_config(uint64_t table_id, DbTableConfig* &cfg);

    std::string &get_output_dir() { return output_dir_; }

    const std::string &get_log_dir() { return log_dir_; }
    const std::string &get_log_level() { return log_level_; }
    std::string &get_ob_log() { return ob_log_dir_; }
    std::string &app_name() { return app_name_; }

    const char *get_host() const { return host_.c_str(); }
    unsigned short get_port() const { return port_; }
    int64_t get_network_timeout() const { return network_timeout_; }

    int64_t get_worker_thread_nr() const { return worker_thread_nr_; }
    int64_t get_parser_thd_nr() const { return parser_thread_nr_; }

    int gen_table_id_map();

    int64_t max_file_size() { return max_file_size_; }
    int64_t rotate_file_interval() { return rotate_file_interval_; }

    std::string &get_tmp_log_path() { return tmp_log_path_; }
    int64_t get_monitor_interval() { return nas_check_interval_; }

    int64_t get_init_log() { return init_log_id_; }

    char get_header_delima() { return header_delima_; }
    char get_body_delima() { return body_delima_; }

    std::string pid_file() { return pid_file_; }
    void destory();

    int64_t max_nolog_interval() { return max_nolog_interval_; }
  private:
    DbDumpConfigMgr() { 
      network_timeout_ = 1000000;
      worker_thread_nr_ = 10;
      parser_thread_nr_ = 1;
      max_file_size_ = 100 * 1024 * 1024; //100M
      rotate_file_interval_ = 10; //10S
      nas_check_interval_ = 1; //1s
      body_delima_ = '\001';
      header_delima_ = '\002';
      pid_file_ = "obdump.pid";
      max_nolog_interval_ = 0;
    }

    std::vector<DbRowFilter *> filter_set_;

    std::vector<DbTableConfig> configs_;
    std::map<uint64_t, DbTableConfig> config_set_;

    static DbDumpConfigMgr *instance_;

    int64_t network_timeout_;

    std::string pid_file_;
    std::string log_dir_;
    std::string log_level_;

    std::string ob_log_dir_;

    //output data path
    std::string output_dir_;

    std::string host_;
    unsigned short port_;
    std::string app_name_;

    //thread nr def
    int64_t worker_thread_nr_;
    int64_t parser_thread_nr_;

    //output file
    int64_t max_file_size_;
    int64_t rotate_file_interval_;

    std::string tmp_log_path_;

    int64_t nas_check_interval_;
    int64_t init_log_id_;

    char header_delima_;
    char body_delima_;

    int64_t monitor_interval_;
    int64_t max_nolog_interval_;                /* seconds */
};

#endif   /* ----- #ifndef ob_api_db_schema_INC  ----- */
