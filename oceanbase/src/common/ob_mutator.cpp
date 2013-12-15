/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_mutator.cpp for ...
 *
 * Authors:
 *   rizhao <rizhao.ych@taobao.com>
 *
 */
#include "ob_mutator.h"

namespace oceanbase
{
  namespace common
  {
    ObMutator :: ObMutator()
    {
      list_head_ = NULL;
      list_tail_ = NULL;
      last_row_key_.assign(NULL, 0);
      last_table_name_.assign(NULL, 0);
      last_table_id_ = 0;
      id_name_type_ = UNSURE;
      cell_store_size_ = 0;
      cur_iter_node_ = NULL;
      has_begin_ = false;
    }

    ObMutator :: ~ObMutator()
    {
    }

    int ObMutator :: reset()
    {
      int err = OB_SUCCESS;
      list_head_ = NULL;
      list_tail_ = NULL;
      last_row_key_.assign(NULL, 0);
      last_table_name_.assign(NULL, 0);
      last_table_id_ = 0;
      id_name_type_ = UNSURE;
      cell_store_size_ = 0;
      cur_iter_node_ = NULL;
      condition_.reset();
      has_begin_ = false;
      err = str_buf_.reset();
      if (OB_SUCCESS == err)
      {
        page_arena_.free();
      }

      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to clear str_buf or page_arena, err=%d", err);
      }

      return err;
    }

    int ObMutator :: use_ob_sem()
    {
      int err = OB_SUCCESS;

      ObMutatorCellInfo mutation;
      mutation.cell_info.value_.set_ext(ObActionFlag::OP_USE_OB_SEM);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add ob sem op, err=%d", err);
      }

      return err;
    }

    int ObMutator :: use_db_sem()
    {
      int err = OB_SUCCESS;

      ObMutatorCellInfo mutation;
      mutation.cell_info.value_.set_ext(ObActionFlag::OP_USE_DB_SEM);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add db sem op, err=%d", err);
      }

      return err;
    }

    int ObMutator :: update(const ObString& table_name, const ObString& row_key,
        const ObString& column_name, const ObObj& value)
    {
      int err = OB_SUCCESS;

      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.column_name_ = column_name;

      mutation.op_type.set_ext(ObActionFlag::OP_UPDATE);
      mutation.cell_info.value_ = value;

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add mutation, err=%d", err);
      }

      return err;
    }

    int ObMutator :: insert(const ObString& table_name, const ObString& row_key,
        const ObString& column_name, const ObObj& value)
    {
      int err = OB_SUCCESS;

      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.column_name_ = column_name;

      mutation.op_type.set_ext(ObActionFlag::OP_INSERT);
      mutation.cell_info.value_ = value;

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add mutation, err=%d", err);
      }

      return err;
    }

    int ObMutator :: add(const ObString& table_name, const ObString& row_key,
        const ObString& column_name, const int64_t value)
    {
      int err = OB_SUCCESS;

      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.column_name_ = column_name;

      mutation.op_type.set_ext(ObActionFlag::OP_UPDATE);
      mutation.cell_info.value_.set_int(value, true);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add mutation, err=%d", err);
      }

      return err;
    }

    int ObMutator :: add(const ObString& table_name, const ObString& row_key,
        const ObString& column_name, const float value)
    {
      int err = OB_SUCCESS;
      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.column_name_ = column_name;

      mutation.op_type.set_ext(ObActionFlag::OP_UPDATE);
      mutation.cell_info.value_.set_float(value, true);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add mutation, err=%d", err);
      }

      return err;
    }

    int ObMutator :: add(const ObString& table_name, const ObString& row_key,
        const ObString& column_name, const double value)
    {
      int err = OB_SUCCESS;
      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.column_name_ = column_name;

      mutation.op_type.set_ext(ObActionFlag::OP_UPDATE);
      mutation.cell_info.value_.set_double(value, true);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add mutation, err=%d", err);
      }

      return err;
    }

    int ObMutator :: add_datetime(const ObString& table_name, const ObString& row_key,
        const ObString& column_name, const ObDateTime& value)
    {
      int err = OB_SUCCESS;
      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.column_name_ = column_name;

      mutation.op_type.set_ext(ObActionFlag::OP_UPDATE);
      mutation.cell_info.value_.set_datetime(value, true);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add mutation, err=%d", err);
      }

      return err;
    }

    int ObMutator :: add_precise_datetime(const ObString& table_name, const ObString& row_key,
        const ObString& column_name, const ObPreciseDateTime& value)
    {
      int err = OB_SUCCESS;
      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.column_name_ = column_name;

      mutation.op_type.set_ext(ObActionFlag::OP_UPDATE);
      mutation.cell_info.value_.set_precise_datetime(value, true);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add mutation, err=%d", err);
      }

      return err;
    }

    int ObMutator :: del_row(const ObString& table_name, const ObString& row_key)
    {
      int err = OB_SUCCESS;
      ObMutatorCellInfo mutation;

      mutation.cell_info.table_name_ = table_name;
      mutation.cell_info.row_key_ = row_key;
      mutation.cell_info.value_.set_ext(ObActionFlag::OP_DEL_ROW);

      err = add_cell(mutation);
      if (OB_SUCCESS != err)
      {
        TBSYS_LOG(WARN, "failed to add del op, err=%d", err);
      }

      return err;
    }

    int ObMutator :: add_cell(const ObMutatorCellInfo& cell)
    {
      int err = OB_SUCCESS;
      int64_t store_size = 0;

      CellInfoNode* cur_node = NULL;
      cur_node = page_arena_.alloc(sizeof(*cur_node));
      if (NULL == cur_node)
      {
        TBSYS_LOG(WARN, "failed to alloc, cur_node=%p", cur_node);
        err = OB_MEM_OVERFLOW;
      }
      else
      {
        err = copy_cell_(cell, cur_node->cell, store_size);
        if (OB_SUCCESS != err)
        {
          TBSYS_LOG(WARN, "failed to copy cell, err=%d", err);
        }
        else
        {
          cur_node->next = NULL;

          err = add_node_(cur_node);
        }
      }

      if (OB_SUCCESS == err)
      {
        cell_store_size_ += store_size;
      }

      return err;
    }

    const ObUpdateCondition& ObMutator::get_update_condition(void) const
    {
      return condition_;
    }

    ObUpdateCondition& ObMutator::get_update_condition(void)
    {
      return condition_;
    }

    void ObMutator :: reset_iter()
    {
      has_begin_ = false;
    }

    int ObMutator :: next_cell()
    {
      int err = OB_SUCCESS;

      if (!has_begin_)
      {
        has_begin_ = true;
        cur_iter_node_ = list_head_;
      }
      else
      {
        if (NULL != cur_iter_node_)
        {
          cur_iter_node_ = cur_iter_node_->next;
        }
      }

      if (NULL == cur_iter_node_)
      {
        err = OB_ITER_END;
      }

      return err;
    }

    int ObMutator :: get_cell(ObMutatorCellInfo** cell)
    {
      int err = OB_SUCCESS;

      if (NULL == cell)
      {
        TBSYS_LOG(WARN, "invalid param, cell=%p", cell);
        err = OB_INVALID_ARGUMENT;
      }
      else if (NULL == cur_iter_node_)
      {
        TBSYS_LOG(WARN, "invalid status, cur_iter_node=%p", cur_iter_node_);
        err = OB_ERROR;
      }
      else
      {
        *cell = &(cur_iter_node_->cell);
      }

      return err;
    }

    int ObMutator :: serialize(char* buf, const int64_t buf_len, int64_t& pos) const
    {
      int err = OB_SUCCESS;
      int64_t ori_pos = pos;

      if (NULL == buf || buf_len <= 0 || pos >= buf_len)
      {
        TBSYS_LOG(WARN, "invalid param, buf=%p, buf_len=%ld, pos=%ld", buf, buf_len, pos);
        err = OB_INVALID_ARGUMENT;
      }
      else
      {
        CellInfoNode* node = list_head_;
        ObString last_table_name;
        ObString last_row_key;
        uint64_t last_table_id = 0;
        last_table_name.assign(NULL, 0);
        last_row_key.assign(NULL, 0);
        int64_t tmp_ext_val = 0;
        ObObj tmp_obj;

        err = serialize_flag_(buf, buf_len, pos, ObActionFlag::MUTATOR_PARAM_FIELD);
        while (OB_SUCCESS == err && NULL != node)
        {
          int64_t type = node->cell.cell_info.value_.get_type();
          if (ObExtendType == type)
          {
            node->cell.cell_info.value_.get_ext(tmp_ext_val);
          }

          if (ObExtendType == type && (ObActionFlag::OP_USE_OB_SEM == tmp_ext_val
              || ObActionFlag::OP_USE_DB_SEM == tmp_ext_val))
          {
            // use ob/db sem
            err = serialize_flag_(buf, buf_len, pos, ObActionFlag::OBDB_SEMANTIC_FIELD);
            if (OB_SUCCESS == err)
            {
              err = node->cell.cell_info.value_.serialize(buf, buf_len, pos);
            }
          }
          else
          {
            // serialize table name or table id
            if (0 != node->cell.cell_info.table_name_.length())
            {
              // serialize table name
              if (0 != node->cell.cell_info.table_name_.compare(last_table_name))
              {
                err = serialize_flag_(buf, buf_len, pos, ObActionFlag::TABLE_NAME_FIELD);
                if (OB_SUCCESS == err)
                {
                  tmp_obj.reset();
                  tmp_obj.set_varchar(node->cell.cell_info.table_name_);
                  err = tmp_obj.serialize(buf, buf_len, pos);
                }
              }
            }
            else
            {
              // serialize table id
              if (last_table_id != node->cell.cell_info.table_id_)
              {
                err = serialize_flag_(buf, buf_len, pos, ObActionFlag::TABLE_NAME_FIELD);
                if (OB_SUCCESS == err)
                {
                  tmp_obj.reset();
                  tmp_obj.set_int(node->cell.cell_info.table_id_);
                  err = tmp_obj.serialize(buf, buf_len, pos);
                }
              }
            }

            if (OB_SUCCESS == err)
            {
              // serialize row key
              if (0 != node->cell.cell_info.row_key_.compare(last_row_key))
              {
                err = serialize_flag_(buf, buf_len, pos, ObActionFlag::ROW_KEY_FIELD);
                if (OB_SUCCESS == err)
                {
                  tmp_obj.reset();
                  tmp_obj.set_varchar(node->cell.cell_info.row_key_);
                  err = tmp_obj.serialize(buf, buf_len, pos);
                }
              }
            }

            if (OB_SUCCESS != err)
            {
              // does nothing
            }
            else if (ObExtendType == type
                && ObActionFlag::OP_DEL_ROW == tmp_ext_val)
            {
              // serialize del row
              err = serialize_flag_(buf, buf_len, pos, ObActionFlag::OP_DEL_ROW);
            }
            else
            {
              // serialize column param
              if (OB_SUCCESS == err)
              {
                // serialize column name or column id
                if (OB_SUCCESS == err)
                {
                  if (0 != node->cell.cell_info.column_name_.length())
                  {
                    tmp_obj.reset();
                    tmp_obj.set_varchar(node->cell.cell_info.column_name_);
                    err = tmp_obj.serialize(buf, buf_len, pos);
                  }
                  else
                  {
                    tmp_obj.reset();
                    tmp_obj.set_int(node->cell.cell_info.column_id_);
                    err = tmp_obj.serialize(buf, buf_len, pos);
                  }
                }

                // serialize op type
                if (OB_SUCCESS == err)
                {
                  err = node->cell.op_type.serialize(buf, buf_len, pos);
                }

                // serialize column value
                if (OB_SUCCESS == err)
                {
                  err = node->cell.cell_info.value_.serialize(buf, buf_len, pos);
                }
              }
            }

            if (OB_SUCCESS == err)
            {
              last_table_name = node->cell.cell_info.table_name_;
              last_table_id = node->cell.cell_info.table_id_;
              last_row_key = node->cell.cell_info.row_key_;
            }
          }

          if (OB_SUCCESS == err)
          {
            node = node->next;
          }
        }

        // serialize update condition
        if (OB_SUCCESS == err)
        {
          // has update condition
          if (condition_.get_count() > 0)
          {
            err = serialize_flag_(buf, buf_len, pos, ObActionFlag::UPDATE_COND_PARAM_FIELD);
            if (OB_SUCCESS == err)
            {
              err = condition_.serialize(buf, buf_len, pos);
            }

            if (OB_SUCCESS != err)
            {
              TBSYS_LOG(WARN, "failed to serialize update condition, err=%d", err);
            }
          }
        }

        // serialize end flag
        if (OB_SUCCESS == err)
        {
          err = serialize_flag_(buf, buf_len, pos, ObActionFlag::END_PARAM_FIELD);
        }

        if (OB_SUCCESS != err)
        {
          TBSYS_LOG(WARN, "failed to serialize, buf=%p, buf_len=%ld, ori_pos=%ld, pos=%ld, err=%d",
              buf, buf_len, ori_pos, pos, err);
        }
      }

      return err;
    }

    int ObMutator :: deserialize(const char* buf, const int64_t buf_len, int64_t& pos)
    {
      int err = OB_SUCCESS;

      if (NULL == buf || buf_len <= 0 || pos >= buf_len)
      {
        TBSYS_LOG(WARN, "invalid param, buf=%p, buf_len=%ld, pos=%ld", buf, buf_len, pos);
        err = OB_INVALID_ARGUMENT;
      }
      else
      {
        // reset ob_mutator
        err = reset();
        if (OB_SUCCESS != err)
        {
          TBSYS_LOG(WARN, "failed to reset, err=%d", err);
        }
      }

      if (OB_SUCCESS == err)
      {
        ObObj op;
        ObObj tmp_obj;
        ObString tmp_str;
        ObMutatorCellInfo cur_cell;
        bool end_flag = false;

        while (OB_SUCCESS == err && pos < buf_len && !end_flag)
        {
          op.reset();
          err = op.deserialize(buf, buf_len, pos);
          if (OB_SUCCESS != err)
          {
            TBSYS_LOG(WARN, "failed to deserialize, err=%d", err);
          }
          else
          {
            int64_t ext_val = 0;
            int64_t type = op.get_type();
            if (ObExtendType == type)
            {
              op.get_ext(ext_val);
            }

            if (ObExtendType != type)
            {
              if (ObIntType == type)
              {
                int64_t column_id = 0;
                op.get_int(column_id);
                cur_cell.cell_info.column_id_ = column_id;
                cur_cell.cell_info.column_name_.assign(NULL, 0);
              }
              else if (ObVarcharType == type)
              {
                op.get_varchar(tmp_str);
                cur_cell.cell_info.column_name_ = tmp_str;
                cur_cell.cell_info.column_id_ = 0;
              }
              else
              {
                TBSYS_LOG(WARN, "invalid obj type, obj_type=%ld", type);
                err = OB_ERROR;
              }

              if (OB_SUCCESS == err)
              {
                // deserialize op type
                cur_cell.op_type.reset();
                err = cur_cell.op_type.deserialize(buf, buf_len, pos);
              }

              if (OB_SUCCESS == err)
              {
                // deserialize column value
                tmp_obj.reset();
                err = tmp_obj.deserialize(buf, buf_len, pos);
                cur_cell.cell_info.value_.reset();
                cur_cell.cell_info.value_ = tmp_obj;
                if (OB_SUCCESS != err)
                {
                  TBSYS_LOG(WARN, "failed to add cell, err=%d", err);
                }
                else
                {
                  err = add_cell(cur_cell);
                  if (OB_SUCCESS != err)
                  {
                    TBSYS_LOG(WARN, "failed to add cell, err=%d", err);
                  }
                }
              }
            }
            else
            {
              switch (ext_val)
              {
                case ObActionFlag::MUTATOR_PARAM_FIELD:
                  // does nothing
                  break;

                case ObActionFlag::OBDB_SEMANTIC_FIELD:
                  // deserialize semantic field
                  err = tmp_obj.deserialize(buf, buf_len, pos);
                  if (OB_SUCCESS == err)
                  {
                    int64_t sem_op = 0;
                    tmp_obj.get_ext(sem_op);
                    ObMutatorCellInfo sem_cell;
                    sem_cell.cell_info.value_.set_ext(sem_op);
                    err = add_cell(sem_cell);
                    if (OB_SUCCESS != err)
                    {
                      TBSYS_LOG(WARN, "failed to add_cell, err=%d", err);
                    }
                  }
                  break;

                case ObActionFlag::TABLE_NAME_FIELD:
                  // deserialize table name
                  err = tmp_obj.deserialize(buf, buf_len, pos);
                  if (OB_SUCCESS == err)
                  {
                    if (ObIntType == tmp_obj.get_type())
                    {
                      int64_t table_id = 0;
                      tmp_obj.get_int(table_id);
                      cur_cell.cell_info.table_id_ = table_id;
                      cur_cell.cell_info.table_name_.assign(NULL, 0);
                    }
                    else if (ObVarcharType == tmp_obj.get_type())
                    {
                      tmp_obj.get_varchar(tmp_str);
                      cur_cell.cell_info.table_name_ = tmp_str;
                      cur_cell.cell_info.table_id_ = 0;
                    }
                    else
                    {
                      TBSYS_LOG(WARN, "invalid obj type, type=%d", tmp_obj.get_type());
                      err = OB_ERROR;
                    }
                  }
                  break;

                case ObActionFlag::ROW_KEY_FIELD:
                  // deserialize row key
                  err = tmp_obj.deserialize(buf, buf_len, pos);
                  if (OB_SUCCESS == err)
                  {
                    tmp_obj.get_varchar(tmp_str);
                    cur_cell.cell_info.row_key_ = tmp_str;
                  }
                  break;

                case ObActionFlag::OP_DEL_ROW:
                  cur_cell.cell_info.column_name_.assign(NULL, 0);
                  cur_cell.cell_info.column_id_ = common::OB_INVALID_ID;
                  cur_cell.cell_info.value_.reset();
                  cur_cell.cell_info.value_.set_ext(ObActionFlag::OP_DEL_ROW);
                  err = add_cell(cur_cell);
                  if (OB_SUCCESS != err)
                  {
                    TBSYS_LOG(WARN, "failed to add cell, err=%d", err);
                  }
                  break;

                case ObActionFlag::UPDATE_COND_PARAM_FIELD:
                  err = condition_.deserialize(buf, buf_len, pos);
                  if (OB_SUCCESS != err)
                  {
                    TBSYS_LOG(WARN, "failed to deserialize update condition, err=%d", err);
                  }
                  break;

                case ObActionFlag::END_PARAM_FIELD:
                  end_flag = true;
                  break;
                default:
                  TBSYS_LOG(WARN, "unknown format, omit it");
                  break;
              }
            }
          }
        }
      }

      return err;
    }

    int64_t ObMutator :: get_serialize_size(void) const
    {
      int64_t store_size = cell_store_size_ + get_obj_serialize_size_(ObActionFlag::MUTATOR_PARAM_FIELD, true)
        + get_obj_serialize_size_(ObActionFlag::END_PARAM_FIELD, true);
      if (condition_.get_count() > 0)
      {
        store_size += get_obj_serialize_size_(ObActionFlag::UPDATE_COND_PARAM_FIELD, true);
        store_size += condition_.get_serialize_size();
      }

      return store_size;
    }

    int ObMutator :: copy_cell_(const ObMutatorCellInfo& src_cell, ObMutatorCellInfo& dst_cell, int64_t& store_size)
    {
      int err = OB_SUCCESS;
      store_size = 0;

      int64_t ext_val = 0;
      int64_t type = src_cell.cell_info.value_.get_type();
      if (ObExtendType == type)
      {
        src_cell.cell_info.value_.get_ext(ext_val);
      }

      if (ObExtendType == type
          && (ObActionFlag::OP_USE_OB_SEM == ext_val
            || ObActionFlag::OP_USE_DB_SEM == ext_val))
      {
        dst_cell.cell_info.reset();
        dst_cell.cell_info.value_.reset();
        dst_cell.cell_info.value_.set_ext(ext_val);
        store_size += (get_obj_serialize_size_(ObActionFlag::OBDB_SEMANTIC_FIELD, true)
            + dst_cell.cell_info.value_.get_serialize_size());
      }
      else
      {
        // store table name or table id
        if (0 != src_cell.cell_info.table_name_.length())
        {
          if (USE_ID == id_name_type_)
          {
            TBSYS_LOG(WARN, "invalid status, should use name");
            err = OB_ERROR;
          }
          else
          {
            id_name_type_ = USE_NAME;
            dst_cell.cell_info.table_id_ = 0;
            // store table name
            if (0 == src_cell.cell_info.table_name_.compare(last_table_name_))
            {
              dst_cell.cell_info.table_name_ = last_table_name_;
            }
            else
            {
              err = str_buf_.write_string(src_cell.cell_info.table_name_,
                  &(dst_cell.cell_info.table_name_));
              if (OB_SUCCESS != err)
              {
                TBSYS_LOG(WARN, "failed to store table name, err=%d", err);
              }
              else
              {
                store_size += (get_obj_serialize_size_(ObActionFlag::TABLE_NAME_FIELD, true)
                    + get_obj_serialize_size_(dst_cell.cell_info.table_name_));
              }
            }
          }
        }
        else
        {
          if (USE_NAME == id_name_type_)
          {
            TBSYS_LOG(WARN, "invalid status, should use id");
            err = OB_ERROR;
          }
          else
          {
            id_name_type_ = USE_ID;
            dst_cell.cell_info.table_name_.assign(NULL, 0);
            // store table id
            if (last_table_id_ == src_cell.cell_info.table_id_)
            {
              dst_cell.cell_info.table_id_ = last_table_id_;
            }
            else
            {
              dst_cell.cell_info.table_id_ = src_cell.cell_info.table_id_;
              store_size += (get_obj_serialize_size_(ObActionFlag::TABLE_NAME_FIELD, true)
                  + get_obj_serialize_size_(dst_cell.cell_info.table_id_, false));
            }
          }
        }

        // store row key
        if (OB_SUCCESS == err)
        {
          if (0 == src_cell.cell_info.row_key_.compare(last_row_key_))
          {
            dst_cell.cell_info.row_key_ = last_row_key_;
          }
          else
          {
            err = str_buf_.write_string(src_cell.cell_info.row_key_,
                &(dst_cell.cell_info.row_key_));
            if (OB_SUCCESS != err)
            {
              TBSYS_LOG(WARN, "failed to store row key, err=%d", err);
            }
            else
            {
              store_size += (get_obj_serialize_size_(ObActionFlag::ROW_KEY_FIELD, true)
                  + get_obj_serialize_size_(dst_cell.cell_info.row_key_));
            }
          }
        }

        if (OB_SUCCESS != err)
        {
          // does nothing
        }
        else if (ObExtendType == type && ObActionFlag::OP_DEL_ROW == ext_val)
        {
          // delete row
          dst_cell.cell_info.column_id_ = common::OB_INVALID_ID;
          dst_cell.cell_info.column_name_.assign(NULL, 0);
          dst_cell.cell_info.value_.reset();
          dst_cell.cell_info.value_.set_ext(ext_val);
          store_size += get_obj_serialize_size_(ObActionFlag::OP_DEL_ROW, true);
        }
        else
        {
          // store column name
          if (0 != src_cell.cell_info.column_name_.length())
          {
            if (USE_ID == id_name_type_)
            {
              TBSYS_LOG(WARN, "invalid status, should use name");
              err = OB_ERROR;
            }
            else
            {
              id_name_type_ = USE_NAME;
              dst_cell.cell_info.column_id_ = 0;

              err = str_buf_.write_string(src_cell.cell_info.column_name_,
                  &(dst_cell.cell_info.column_name_));
              if (OB_SUCCESS != err)
              {
                TBSYS_LOG(WARN, "failed to store column name, err=%d", err);
              }
              else
              {
                store_size += get_obj_serialize_size_(dst_cell.cell_info.column_name_);
              }
            }
          }
          else
          {
            if (USE_NAME == id_name_type_)
            {
              TBSYS_LOG(WARN, "invalid status, should use id");
              err = OB_ERROR;
            }
            else
            {
              id_name_type_ = USE_ID;
              dst_cell.cell_info.column_name_.assign(NULL, 0);
              //store column id
              dst_cell.cell_info.column_id_ = src_cell.cell_info.column_id_;
              store_size += get_obj_serialize_size_(dst_cell.cell_info.column_id_, false);
            }
          }

          // store op type
          if (OB_SUCCESS == err)
          {
            dst_cell.op_type = src_cell.op_type;
            store_size += dst_cell.op_type.get_serialize_size();
          }

          if (OB_SUCCESS == err)
          {
            // store column value
            err = str_buf_.write_obj(src_cell.cell_info.value_, &(dst_cell.cell_info.value_));
            if (OB_SUCCESS != err)
            {
              TBSYS_LOG(WARN, "failed to store column value, err=%d", err);
            }
            else
            {
              store_size += dst_cell.cell_info.value_.get_serialize_size();
            }
          }
        }

        if (OB_SUCCESS == err)
        {
          last_table_name_ = dst_cell.cell_info.table_name_;
          last_row_key_ = dst_cell.cell_info.row_key_;
          last_table_id_ = dst_cell.cell_info.table_id_;
        }
      }

      return err;
    }

    int ObMutator :: add_node_(CellInfoNode* cur_node)
    {
      int err = OB_SUCCESS;

      if (NULL == cur_node)
      {
        TBSYS_LOG(WARN, "invalid param, cur_node=%p", cur_node);
        err = OB_INVALID_ARGUMENT;
      }
      else
      {
        if (NULL == list_head_)
        {
          list_head_ = cur_node;
          list_tail_ = cur_node;
        }
        else
        {
          list_tail_->next = cur_node;
          list_tail_ = cur_node;
        }
      }

      return err;
    }

    int ObMutator::serialize_flag_(char* buf, const int64_t buf_len, int64_t& pos,
        const int64_t flag) const
    {
      int err = OB_SUCCESS;

      ObObj obj;
      obj.set_ext(flag);

      err = obj.serialize(buf, buf_len, pos);

      return err;
    }

    int64_t ObMutator::get_obj_serialize_size_(const int64_t value, bool is_ext) const
    {
      ObObj obj;
      if (is_ext)
      {
        obj.set_ext(value);
      }
      else
      {
        obj.set_int(value);
      }

      return obj.get_serialize_size();
    }

    int64_t ObMutator::get_obj_serialize_size_(const ObString& str) const
    {
      ObObj obj;
      obj.set_varchar(str);

      return obj.get_serialize_size();
    }
  }
}


