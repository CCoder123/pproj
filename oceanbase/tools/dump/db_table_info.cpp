/*
 * =====================================================================================
 *
 *       Filename:  DbTableInfo.cpp
 *
 *        Version:  1.0
 *        Created:  04/13/2011 10:14:35 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yushun.swh@taobao.com
 *        Company:  taobao
 *
 * =====================================================================================
 */
#include "db_table_info.h"
#include "db_record.h"
#include "common/murmur_hash.h"
#include <string>
#include <sstream>


namespace oceanbase {
  namespace api {
    const char* CS_PORT =   "port";
    const char* MS_PORT =   "ms_port";
    const char* CS_IPV4   = "ipv4";
    const char* TABLET_VERSION= "tablet_version";

    const char* ROOT_OCCUPY_SIZE ="occupy_size";
    const char* ROOT_RECORD_COUNT ="record_count";
    const char* ROOT_CRC_SUM ="crc_sum";

    TabletInfo::TabletInfo(void) 
    {
    }

    TabletInfo::~TabletInfo() 
    {
    }

    TabletInfo::TabletInfo(const TabletInfo &src)
    {
      for(int i = 0;i < kTabletDupNr; i++) {
        slice_[i] = src.slice_[i];
      }

      rowkey_buffer_ = src.rowkey_buffer_;
    }

    DbRowKey TabletInfo::get_end_key()
    {
      DbRowKey end_key;
      end_key.assign_ptr(const_cast<char*>(rowkey_buffer_.data()), rowkey_buffer_.length());
      return end_key;
    }

    TabletInfo& TabletInfo::operator=(const TabletInfo &src)
    {
      for(int i = 0;i < kTabletDupNr; i++) {
        slice_[i] = src.slice_[i];
      }

      rowkey_buffer_ = src.rowkey_buffer_;
      return *this;
    }

    int TabletInfo::assign_end_key(const DbRowKey &key) 
    {
      rowkey_buffer_.assign(key.ptr(), key.length());
      return common::OB_SUCCESS;
    }

    int TabletInfo::parse_one_cell(const common::ObCellInfo *cell) 
    {
      common::ObString key = cell->column_name_; 
      int64_t value;
      int ret;

      if ((ret = cell->value_.get_int(value)) != common::OB_SUCCESS) {
        TBSYS_LOG(ERROR, "Cell->value_.get_int failed");
        return ret;
      }

      if (!key.compare(ROOT_OCCUPY_SIZE)) {
        ocuppy_size_ = value;
      } else if (!key.compare(ROOT_RECORD_COUNT)) {
        record_count_ = value;
      } else if (!key.compare(ROOT_CRC_SUM)) {
        //TODO:crc sum
      } else {
        int idx = *key.ptr() - '1';
        if (idx < 0 || idx > 2) {
          TBSYS_LOG(ERROR, "Tablet slice number is wrong");
          return -1;
        }

        //skip 'idx' + '_'
        key.assign_ptr(key.ptr() + 2, key.length() - 2);
        if (!key.compare(CS_PORT)) {
          slice_[idx].cs_port = (unsigned short)value;
        } else if (!key.compare(MS_PORT)) {
          slice_[idx].ms_port = (unsigned short)value; 
        } else if (!key.compare(CS_IPV4)) {
          slice_[idx].ip_v4 = (int32_t)value;
        } else if (!key.compare(TABLET_VERSION)) {
          slice_[idx].tablet_version = value;
        }
      }
      return 0;
    }

    void TabletInfo::dump_slice(void) 
    {
      char buf[1024];
      int len = hex_to_str(get_end_key().ptr(), get_end_key().length(), buf, 1024);
      buf[len * 2] = '\0';

      for(int i = 0; i < 3; i++) {
        TBSYS_LOG(INFO, "ipv4:%x, cs_port:%d, ms_port:%d, tablet_vesion:%ld, avail:%d, key:%s", 
                  slice_[i].ip_v4, slice_[i].cs_port, slice_[i].ms_port, 
                  slice_[i].tablet_version, slice_[i].server_avail, buf);
      }
    }

    int TabletInfo::get_tablet_location(int idx, TabletSliceLocation &loc) 
    {
      if (idx < 0 || idx > 2)
        return common::OB_ERROR;

      loc = slice_[idx];
      return common::OB_SUCCESS;
    }

    int TabletInfo::get_one_avail_slice(TabletSliceLocation &loc, const DbRowKey &rowkey)
    {
      //TODO:dispatch requset to different server
      int i;
      int idx = common::murmurhash2((void *)rowkey.ptr(), rowkey.length(), 0) % 3;

      for (i = 0; i < kTabletDupNr; i++) {
        if (slice_[idx].server_avail) {
          loc = slice_[idx];
          //          dump_slice();
          break;
        } else {
          idx++;
          if (idx >= 3)
            idx -= 3 ;
        }
      }

      if (i != kTabletDupNr)
        return common::OB_SUCCESS;
      else 
        return common::OB_ERROR;
    }

    int TabletInfo::parse_from_record(DbRecord *recp)
    {
      int ret = OB_SUCCESS;
      ObCellInfo *cell;
      bool rowkey_assigned = false;
      int64_t value = 0;

      for (int i = 0;i < kTabletDupNr;i++) {
        TabletSliceLocation location;
        std::string port, ms_port, version, ipv4;
        std::stringstream ss;

        ss << i + 1;
        ss << "_port";
        ss >> port;
        ss.clear();

        ss << i + 1;
        ss << "_ms_port";
        ss >> ms_port;
        ss.clear();

        ss << i + 1;
        ss << "_tablet_version";
        ss >> version;
        ss.clear();

        ss << i + 1;
        ss << "_ipv4";
        ss >> ipv4;
        ss.clear();

        ret = recp->get(port, &cell);
        if (ret != OB_SUCCESS || cell->value_.get_int(value) != OB_SUCCESS) {
//          TBSYS_LOG(WARN, "error when get cs_port");
          continue;
        }
        location.cs_port = (unsigned short)value;

        ret = recp->get(ms_port, &cell);
        if (ret != OB_SUCCESS || cell->value_.get_int(value) != OB_SUCCESS) {
//          TBSYS_LOG(INFO, "error when get ms_port");
          continue;
        }
        location.ms_port = (unsigned short)value;

        ret = recp->get(version, &cell);
        if (ret != common::OB_SUCCESS ||
            cell->value_.get_int(value)) {
//          TBSYS_LOG(INFO, "error when get version");
          continue;
        }
        location.tablet_version = value;

        ret = recp->get(ipv4, &cell);
        if (ret != common::OB_SUCCESS ||
            cell->value_.get_int(value) != common::OB_SUCCESS) {
//          TBSYS_LOG(INFO, "error when get ipv4");
          continue;
        }
        location.ip_v4 = (uint32_t)value;

        location.server_avail = true;
        if (!rowkey_assigned) {
          assign_end_key(cell->row_key_);
          rowkey_assigned = true;
        }

        slice_[i] = location;
      } // end of for

      ret = recp->get("occupy_size", &cell);
      if (ret != common::OB_SUCCESS ||
          cell->value_.get_int(value) != common::OB_SUCCESS) {
//        TBSYS_LOG(INFO, "error when get occupy_size");
        ret = OB_ERROR;
      } else {
        ocuppy_size_ = value;

        ret = recp->get("record_count", &cell);
        if (ret != common::OB_SUCCESS ||
            cell->value_.get_int(value)) {
          TBSYS_LOG(INFO, "error when get record_count");
          ret = OB_ERROR;
        } else
          record_count_ = value;
      }

      dump_slice();
      return ret;
    }
  }
}
