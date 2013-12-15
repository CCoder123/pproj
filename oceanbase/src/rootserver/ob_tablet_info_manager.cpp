/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_tablet_info_manager.cpp for ...
 *
 * Authors:
 *   daoan <daoan@taobao.com>
 *
 */

#include "rootserver/ob_tablet_info_manager.h"
#include "common/utility.h"
namespace oceanbase
{
  namespace rootserver
  {
    using namespace common;
    ObTabletCrcHistoryHelper::ObTabletCrcHistoryHelper()
    {
      reset();
    }
    void ObTabletCrcHistoryHelper::reset()
    {
      memset(version_, 0, sizeof(int64_t) * MAX_KEEP_HIS_COUNT);
      rest_all_crc_sum();
    }
    void ObTabletCrcHistoryHelper::rest_all_crc_sum()
    {
      memset(crc_sum_, 0, sizeof(uint64_t) * MAX_KEEP_HIS_COUNT);
    }
    int ObTabletCrcHistoryHelper::check_and_update(const int64_t version, const uint64_t crc_sum)
    {
      int ret = OB_SUCCESS;
      int64_t min_version = 0;
      int64_t max_version = 0;
      get_min_max_version(min_version, max_version);
      if (version > max_version) 
      {
        update_crc_sum(min_version, version, crc_sum);
      }
      else 
      {
        bool found_same_version = false;
        for (int i = 0; i < MAX_KEEP_HIS_COUNT; i++)
        {
          if (version_[i] == version)
          {
            found_same_version = true;
            if (crc_sum_[i] == 0)
            {
              crc_sum_[i] = crc_sum;
            }
            else
            {
              if (crc_sum != 0 && crc_sum != crc_sum_[i])
              {
                TBSYS_LOG(WARN, "crc check error version %ld crc sum %lu, %lu", version_[i], crc_sum_[i], crc_sum);
                ret = OB_ERROR;
              }
            }
          }
        }
        if (!found_same_version)
        {
          if (version > min_version) update_crc_sum(min_version, version, crc_sum);
        }
      }
      return ret;
    }
    void ObTabletCrcHistoryHelper::get_min_max_version(int64_t& min_version, int64_t& max_version) const
    {
      min_version = version_[0];
      max_version = 0;
      for (int i = 0; i < MAX_KEEP_HIS_COUNT; i++)
      {
        if (version_[i] < min_version) min_version = version_[i];
        if (version_[i] > max_version) max_version = version_[i];
      }
    }
    DEFINE_SERIALIZE(ObTabletCrcHistoryHelper)
    {
      int ret = 0;
      int64_t tmp_pos = pos;
      for (int i = 0; i < MAX_KEEP_HIS_COUNT; i++)
      {
        if (OB_SUCCESS == ret)
        {
          ret = serialization::encode_vi64(buf, buf_len, tmp_pos, version_[i]);
        }
        if (OB_SUCCESS == ret)
        {
          ret = serialization::encode_vi64(buf, buf_len, tmp_pos, crc_sum_[i]);
        }
      }
      if (OB_SUCCESS == ret)
      {
        pos = tmp_pos;
      }
      return ret;
    }
    DEFINE_DESERIALIZE(ObTabletCrcHistoryHelper)
    {
      int ret = OB_SUCCESS;      
      int64_t tmp_pos = pos;
      for (int i = 0; i < MAX_KEEP_HIS_COUNT; i++)
      {
        if (OB_SUCCESS == ret)
        {
          ret = serialization::decode_vi64(buf, data_len, tmp_pos, &version_[i]);
        }
        if (OB_SUCCESS == ret)
        {
          ret = serialization::decode_vi64(buf, data_len, tmp_pos, reinterpret_cast<int64_t *>(&crc_sum_[i]));
        }
      }
      if (OB_SUCCESS == ret)
      {

        pos = tmp_pos;
      }
      return ret;
    }
    DEFINE_GET_SERIALIZE_SIZE(ObTabletCrcHistoryHelper)
    {
      int64_t len = 0;
      for (int i = 0; i < MAX_KEEP_HIS_COUNT; i++)
      {
        len += serialization::encoded_length_vi64(version_[i]);
        len += serialization::encoded_length_vi64(crc_sum_[i]);
      }
      return len;
    }
    void ObTabletCrcHistoryHelper::update_crc_sum(const int64_t version, const int64_t new_version, const uint64_t crc_sum)
    {
      for (int i = 0; i < MAX_KEEP_HIS_COUNT; i++)
      {
        if (version_[i] == version) 
        {
          version_[i] = new_version;
          crc_sum_[i] = crc_sum;
          break;
        }
      }
    }
    void ObTabletCrcHistoryHelper::reset_crc_sum(const int64_t version)
    {
      for (int i = 0 ; i < MAX_KEEP_HIS_COUNT; i++)
      {
        if (version_[i] == version) crc_sum_[i] = 0;
      }
    }
    ObTabletInfoManager::ObTabletInfoManager()
    {
      tablet_infos_.init(MAX_TABLET_COUNT, data_holder_);
    }
    int ObTabletInfoManager::add_tablet_info(const common::ObTabletInfo& tablet_info, int32_t& out_index,
        bool clone_start_key, bool clone_end_key )
    {
      out_index = OB_INVALID_INDEX;
      int32_t ret = OB_SUCCESS;
      if (tablet_infos_.get_array_index() >= MAX_TABLET_COUNT)
      {
        TBSYS_LOG(ERROR, "too many tablet max is %d now is %ld", MAX_TABLET_COUNT, tablet_infos_.get_array_index());
        ret = OB_ERROR;
      }
      if (OB_SUCCESS == ret)
      {
        ObTabletInfo tmp;
        clone_tablet(tmp, tablet_info, clone_start_key, clone_end_key);
        if(!tablet_infos_.push_back(tmp))
        {
          TBSYS_LOG(ERROR, "too many tablet max is %d now is %ld", MAX_TABLET_COUNT, tablet_infos_.get_array_index());
          ret = OB_ERROR;
        }
        else
        {
          out_index = tablet_infos_.get_array_index() - 1;
        }
      }
      return ret;
    }
    int32_t ObTabletInfoManager::get_index(common::ObTabletInfo* data_pointer) const
    {
      return data_pointer - begin();
    }
    const ObTabletInfo* ObTabletInfoManager::get_tablet_info(const int32_t index) const
    {
      return tablet_infos_.at(index);
    }
    ObTabletInfo* ObTabletInfoManager::get_tablet_info(const int32_t index)
    {
      return tablet_infos_.at(index);
    }
    const ObTabletInfo* ObTabletInfoManager::begin() const
    {
      return data_holder_;
    }
    ObTabletInfo* ObTabletInfoManager::begin()
    {
      return data_holder_;
    }
    const ObTabletInfo* ObTabletInfoManager::end() const
    {
      return begin() + tablet_infos_.get_array_index();
    }
    ObTabletInfo* ObTabletInfoManager::end() 
    {
      return begin() + tablet_infos_.get_array_index();
    }
    int32_t ObTabletInfoManager::get_array_size() const
    {
      return tablet_infos_.get_array_index();
    }

    void ObTabletInfoManager::clone_tablet(common::ObTabletInfo& dst, const common::ObTabletInfo& src,
        bool clone_start_key, bool clone_end_key)
    {   
      dst.row_count_ = src.row_count_;
      dst.occupy_size_ = src.occupy_size_;
      dst.crc_sum_ = src.crc_sum_;
      dst.range_.table_id_ = src.range_.table_id_;
      dst.range_.border_flag_ = src.range_.border_flag_;


      if (clone_start_key) 
      {
        common::ObString::obstr_size_t sk_len = src.range_.start_key_.length();
        char* sk = reinterpret_cast<char*>(allocator_.alloc(sk_len));
        memcpy(sk, src.range_.start_key_.ptr(), sk_len);
        dst.range_.start_key_.assign(sk, sk_len);
      }
      else
      {
        dst.range_.start_key_ = src.range_.start_key_;
      }

      if (clone_end_key)
      {
        common::ObString::obstr_size_t ek_len = src.range_.end_key_.length();
        char* ek = reinterpret_cast<char*>(allocator_.alloc(ek_len));
        memcpy(ek, src.range_.end_key_.ptr(), ek_len);
        dst.range_.end_key_.assign(ek, ek_len);
      }
      else
      {
        dst.range_.end_key_ = src.range_.end_key_;
      }
    }   

    ObTabletCrcHistoryHelper *ObTabletInfoManager::get_crc_helper(const int32_t index)
    {
      ObTabletCrcHistoryHelper *ret = NULL;
      if (index >= 0 && index < tablet_infos_.get_array_index())
      {
        ret = &crc_helper_[index];
      }
      return ret;
    }

    const ObTabletCrcHistoryHelper *ObTabletInfoManager::get_crc_helper(const int32_t index) const
    {
      const ObTabletCrcHistoryHelper *ret = NULL;
      if (index >= 0 && index < tablet_infos_.get_array_index())
      {
        ret = &crc_helper_[index];
      }
      return ret;
    }
    void ObTabletInfoManager::hex_dump(const int32_t index, const int32_t log_level) const
    {
      static char row_key_dump_buff[OB_MAX_ROW_KEY_LENGTH * 2];
      if (index >= 0 && index < tablet_infos_.get_array_index() && TBSYS_LOGGER._level >= log_level)
      {
        data_holder_[index].range_.to_string(row_key_dump_buff, OB_MAX_ROW_KEY_LENGTH * 2);
        TBSYS_LOG(INFO, "%s", row_key_dump_buff);
        TBSYS_LOG(INFO, "row_count = %ld occupy_size = %ld", 
            data_holder_[index].row_count_, data_holder_[index].occupy_size_);
        for (int i = 0; i < ObTabletCrcHistoryHelper::MAX_KEEP_HIS_COUNT; i++)
        {
          TBSYS_LOG(INFO, "crc_info %d version %ld, crc_sum %lu", 
              i, crc_helper_[index].version_[i], crc_helper_[index].crc_sum_[i]);
        }
      }

    }

    void ObTabletInfoManager::dump_as_hex(FILE* stream) const
    {
      static char start_key_buff[OB_MAX_ROW_KEY_LENGTH * 2];
      static char end_key_buff[OB_MAX_ROW_KEY_LENGTH * 2];
      int size = tablet_infos_.get_array_index();
      fprintf(stream, "size %d", size);
      for (int64_t i = 0; i < tablet_infos_.get_array_index(); i++)
      {
        fprintf(stream, "index %ld row_count %ld occupy_size %ld crcinfo %lu ", 
            i, data_holder_[i].row_count_, data_holder_[i].occupy_size_, data_holder_[i].crc_sum_ );
        for (int crc_i = 0; crc_i < ObTabletCrcHistoryHelper::MAX_KEEP_HIS_COUNT; crc_i++)
        {
          fprintf(stream, "%ld %lu ", crc_helper_[i].version_[crc_i], crc_helper_[i].crc_sum_[crc_i]);
        }
        fprintf(stream, "table_id %ld border_flag %d\n",
            data_holder_[i].range_.table_id_, data_holder_[i].range_.border_flag_.get_data());

        start_key_buff[0] = 0;
        end_key_buff[0] = 0;

        if (data_holder_[i].range_.start_key_.ptr() != NULL)
        {
          common::hex_to_str(data_holder_[i].range_.start_key_.ptr(), data_holder_[i].range_.start_key_.length(),
              start_key_buff, OB_MAX_ROW_KEY_LENGTH * 2);
        }

        if (data_holder_[i].range_.end_key_.ptr() != NULL)
        {
          common::hex_to_str(data_holder_[i].range_.end_key_.ptr(), data_holder_[i].range_.end_key_.length(),
              end_key_buff, OB_MAX_ROW_KEY_LENGTH * 2);
        }

        fprintf(stream, "start_key %d %s\n end_key %d %s\n", data_holder_[i].range_.start_key_.length(), start_key_buff, 
            data_holder_[i].range_.end_key_.length(), end_key_buff);
      }
      return;
    }
    void ObTabletInfoManager::read_from_hex(FILE* stream)
    {
      static char start_key_buff[OB_MAX_ROW_KEY_LENGTH * 2];
      static char end_key_buff[OB_MAX_ROW_KEY_LENGTH * 2];
      static char ob_start_key_buff[OB_MAX_ROW_KEY_LENGTH];
      static char ob_end_key_buff[OB_MAX_ROW_KEY_LENGTH];

      int size = 0;
      fscanf(stream, "size %d", &size);
      int64_t index = 0;
      int32_t out_index = 0;
      for (int64_t i = 0; i < size; i++)
      {
        ObTabletInfo tablet_info;
        int border_flag_data = 0;

        fscanf(stream, "index %ld row_count %ld occupy_size %ld crcinfo %lu ",
            &index, &tablet_info.row_count_, &tablet_info.occupy_size_, &tablet_info.crc_sum_);
        for (int crc_i = 0; crc_i < ObTabletCrcHistoryHelper::MAX_KEEP_HIS_COUNT; crc_i++)
        {
          fscanf(stream, "%ld %lu ", &crc_helper_[i].version_[crc_i], &crc_helper_[i].crc_sum_[crc_i]);
        }
        fscanf(stream, "table_id %ld border_flag %d\n", &tablet_info.range_.table_id_, &border_flag_data);

        tablet_info.range_.border_flag_.set_data(border_flag_data);

        start_key_buff[0] = 0;
        end_key_buff[0] = 0;
        ob_start_key_buff[0] = 0;
        ob_end_key_buff[0] = 0;

        int len = 0;
        fscanf(stream, "start_key %d", &len);
        if (len > 0)
        {
          fscanf(stream, "%s", start_key_buff);
        }

        len = 0;
        fscanf(stream, "\nend_key %d", &len);

        if (len > 0)
        {
          fscanf(stream, "%s", end_key_buff);
        }
        fscanf(stream, "\n");

        str_to_hex(start_key_buff, strlen(start_key_buff), ob_start_key_buff, OB_MAX_ROW_KEY_LENGTH);
        str_to_hex(end_key_buff, strlen(end_key_buff), ob_end_key_buff, OB_MAX_ROW_KEY_LENGTH);

        tablet_info.range_.start_key_.assign_ptr(ob_start_key_buff, strlen(start_key_buff)/2 );
        tablet_info.range_.end_key_.assign_ptr(ob_end_key_buff, strlen(end_key_buff)/2 );

        add_tablet_info(tablet_info, out_index);
        if (out_index != index)
        {
          TBSYS_LOG(ERROR, "index %ld != out_index %d", index, out_index);
          break;
        }
      }
      return;
    }
    DEFINE_SERIALIZE(ObTabletInfoManager)
    {
      int ret = 0;
      int64_t tmp_pos = pos;
      if (OB_SUCCESS == ret)
      {
        ret = serialization::encode_vi64(buf, buf_len, tmp_pos, tablet_infos_.get_array_index());
      }
      for (int64_t i = 0; i < tablet_infos_.get_array_index(); i++)
      {
        ret = data_holder_[i].serialize(buf, buf_len, tmp_pos);
        if (OB_SUCCESS != ret)
        {
          break;
        }
        ret = crc_helper_[i].serialize(buf, buf_len, tmp_pos);
        if (OB_SUCCESS != ret)
        {
          break;
        }
      }
      if (OB_SUCCESS == ret)
      {
        pos = tmp_pos;
      }
      return ret;
    }
    DEFINE_DESERIALIZE(ObTabletInfoManager)
    {
      int ret = OB_SUCCESS;      
      int64_t tmp_pos = pos;
      int64_t total_size = 0;
      if (OB_SUCCESS == ret)
      {
        ret = serialization::decode_vi64(buf, data_len, tmp_pos, &total_size);
      }
      if (OB_SUCCESS == ret)
      {
        for (int64_t i = 0; i < total_size; i++)
        {
          ObTabletInfo tmp_tablet_info;
          int32_t out_index;
          ret = tmp_tablet_info.deserialize(buf, data_len, tmp_pos);
          add_tablet_info(tmp_tablet_info, out_index);
          ret = crc_helper_[out_index].deserialize(buf, data_len, tmp_pos);
          if (OB_SUCCESS != ret) {
            break;
          }
        }
      }
      if (OB_SUCCESS == ret)
      {

        pos = tmp_pos;
      }
      return ret;
    }
    DEFINE_GET_SERIALIZE_SIZE(ObTabletInfoManager)
    {
      int64_t len = serialization::encoded_length_vi64(tablet_infos_.get_array_index());
      for (int64_t i = 0; i < tablet_infos_.get_array_index(); i++)
      {
        len += data_holder_[i].get_serialize_size();
        len += crc_helper_[i].get_serialize_size();

      }
      return len;
    }
  }
}

