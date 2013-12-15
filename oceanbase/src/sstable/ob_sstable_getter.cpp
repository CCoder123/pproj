/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *  
 * Version: 5567
 *
 * ob_sstable_getter.cpp
 *
 * Authors:
 *     huating <huating.zmq@taobao.com>
 *
 */
#include "common/utility.h"
#include "common/ob_define.h"
#include "common/ob_record_header.h"
#include "common/ob_action_flag.h"
#include "ob_sstable_reader.h"
#include "ob_sstable_getter.h"
#include "ob_sstable_writer.h"
#include "ob_blockcache.h"

namespace oceanbase
{
  namespace sstable
  {
    using namespace common;

    ObSSTableGetter::ObSSTableGetter() 
    : inited_(false), cur_state_(ITERATE_NORMAL), readers_(NULL), 
      readers_size_(0), cur_reader_idx_(0), prev_reader_idx_(-1),  
      handled_cells_(0), column_group_num_(0), cur_column_group_idx_(0), 
      get_param_(NULL), getter_(cur_column_mask_), block_cache_(NULL), 
      block_index_cache_(NULL), uncomp_buf_(DEFAULT_UNCOMP_BUF_SIZE) 
    {
      memset(column_group_, 0, OB_MAX_COLUMN_GROUP_NUMBER * sizeof(uint64_t));
      memset(&block_pos_, 0, sizeof(block_pos_));
    }

    ObSSTableGetter::~ObSSTableGetter()
    {

    }

    int ObSSTableGetter::check_internal_member()
    {
      int ret           = OB_SUCCESS;
      int64_t cell_size = 0;
      int64_t row_size  = 0;
      const ObGetParam::ObRowIndex* row_index = NULL;

      if (!inited_)
      {
        TBSYS_LOG(WARN, "ObSSTableGetter doesn't initialize");
        ret = OB_ERROR;
      }
      else if (NULL == get_param_ || NULL == readers_ || readers_size_ <= 0)
      {
        TBSYS_LOG(WARN, "get_param or or readers is NULL, get_param=%p, "
                        "readers=%p, readers_size=%d",
                  get_param_, readers_, readers_size_);
        ret = OB_ERROR;
      }

      if (OB_SUCCESS == ret)
      {
        cell_size = get_param_->get_cell_size();
        row_index = get_param_->get_row_index();
        row_size = get_param_->get_row_size();
        if (cell_size <= 0 || NULL == row_index || row_size <= 0)
        {
          TBSYS_LOG(WARN, "invalid get_param, cell_size=%ld row_index=%p, "
                          "row_size=%ld",
                    cell_size, row_index, row_size);
          ret = OB_ERROR;
        }
        else if (cur_reader_idx_ >= readers_size_)
        {
          TBSYS_LOG(WARN, "current reader index overflow, cur_reader_idx_=%d, "
                          "readers_size_=%d", cur_reader_idx_, readers_size_);
          ret = OB_ERROR;
        }
      }

      return ret;
    }

    int ObSSTableGetter::switch_row()
    {
      int ret               = OB_SUCCESS;
      bool need_load_block  = false;
      int64_t cell_size     = 0;
      const ObGetParam::ObRowIndex* row_index = NULL;

      cur_state_ = ITERATE_NORMAL;  //reset current state first
      if (cur_reader_idx_ >= readers_size_)
      {
        cur_state_ = ITERATE_END;
        ret = OB_ITER_END;
      }
      else
      {
        cell_size = get_param_->get_cell_size();
        row_index = get_param_->get_row_index();

        //all column groups of current row is traversed, shift to next row
        if (++cur_column_group_idx_ >= column_group_num_)
        {
          handled_cells_ += row_index[cur_reader_idx_].size_;
          if (cur_reader_idx_ + 1 < readers_size_)
          {
            cur_reader_idx_++;
            column_group_num_ = 0;
            cur_column_group_idx_ = 0;
  
            ret = filter_column_group();
            if (OB_SUCCESS == ret)
            {
              need_load_block = true;
            }
            else if (OB_SEARCH_NOT_FOUND == ret)
            {
              cur_state_ = ROW_NOT_FOUND;
              ret = OB_GET_NEXT_ROW;
            }
          }
          else
          {
            // end iteration
            cur_state_ = ITERATE_END;
            ret = OB_ITER_END;
          }
        }
        else
        {
          need_load_block = true;
        }

        if (need_load_block)
        {
          //shift to next column group of current row, load current block
          ret = load_cur_block();
          if (OB_SEARCH_NOT_FOUND == ret)
          {
            cur_state_ = ROW_NOT_FOUND;
            ret = OB_GET_NEXT_ROW;
          }
          else if (OB_SUCCESS == ret)
          {
            ret = OB_GET_NEXT_ROW;
          }
          else if (OB_SUCCESS != ret)
          {
            TBSYS_LOG(WARN, "failed to load current block, ret=%d", ret);
          }
        }
      }

      return ret;
    }

    const ObCellInfo* ObSSTableGetter::get_cur_param_cell() const
    {
      const ObCellInfo* cell  = NULL;
      const ObGetParam::ObRowIndex* row_index = NULL;

      row_index = get_param_->get_row_index();
      cell = (*get_param_)[row_index[cur_reader_idx_].offset_];
      if (NULL == cell)
      {
        TBSYS_LOG(WARN, "crrent cell in get param is NULL, get_param=%p, "
                        "row_index=%p, cur_reader_idx_=%d, cell_size=%ld, "
                        "cur_index=%d",
                  get_param_, row_index, cur_reader_idx_, get_param_->get_cell_size(), 
                  row_index[cur_reader_idx_].offset_);
      }

      return cell;
    }

    int ObSSTableGetter::handle_row_not_found()
    {
      int ret                 = OB_SUCCESS;
      const ObCellInfo* cell  = NULL;

      cell = get_cur_param_cell();
      if (NULL == cell)
      {
        TBSYS_LOG(WARN, "crrent cell in get param is NULL");
        ret = OB_ERROR;
      }

      if (OB_SUCCESS == ret)
      {
        /**
         * the rowkey is non-existent, only return one cell to show 
         * this row is non-existent 
         */
        if (0 != cur_column_group_idx_)
        {
          TBSYS_LOG(WARN, "row key doesn't exist in current column group, but it "
                          "exists in the other column group, cur_column_group_idx_=%d",
                    cur_column_group_idx_);
          hex_dump(cell->row_key_.ptr(), cell->row_key_.length(), true, 
                   TBSYS_LOG_LEVEL_WARN);
          ret = OB_ERROR;
        }
        else
        {
          cur_column_group_idx_ = column_group_num_ - 1;
          ret = OB_SUCCESS;
        }
      }

      return ret;
    }

    int ObSSTableGetter::switch_column()
    {
      int ret = OB_SUCCESS;

      //get the next column for current row
      switch (cur_state_)
      {
      case ITERATE_NORMAL:
        if (OB_ITER_END == (ret = getter_.next_cell()))
        {
          ret = OB_GET_NEXT_ROW;
        }
        break;
      case ROW_NOT_FOUND:
        ret = handle_row_not_found();
        break;
      case GET_NEXT_ROW:
        ret = OB_GET_NEXT_ROW;
        break;
      case ITERATE_END:
        ret = OB_ITER_END;
        break;
      default:
        ret = OB_ITER_END;
        break;
      }

      return ret;
    }
    
    int ObSSTableGetter::next_cell()
    {
      int ret = OB_SUCCESS;
      
      ret = check_internal_member();
      if (OB_SUCCESS == ret)
      {
        do
        {
          ret = switch_column();
          if (OB_GET_NEXT_ROW == ret)
          {
            //have trvase one column group row, shift to next row of column group
            ret = switch_row();
            if (OB_SUCCESS == ret)
            {
              ret = switch_column();
            }
          }
        }
        while (OB_GET_NEXT_ROW == ret);
      }

      return ret;
    }

    int ObSSTableGetter::get_cell(ObCellInfo** cell_info)
    {
      return get_cell(cell_info, NULL);
    }

    int ObSSTableGetter::get_cell(ObCellInfo** cell_info, bool* is_row_changed)
    {
      int ret                 = OB_SUCCESS;
      const ObCellInfo* cell  = NULL;
      ObObj obj;

      if (NULL == cell_info)
      {
        TBSYS_LOG(WARN, "invalid param, cell_info=%p", cell_info);
        ret = OB_INVALID_ARGUMENT;
      }
      else if (OB_SUCCESS == ret 
               && OB_SUCCESS == (ret = check_internal_member()) )
      {
        cell = get_cur_param_cell();
        if (NULL == cell)
        {
          TBSYS_LOG(WARN, "crrent cell in get param is NULL");
          ret = OB_ERROR;
        }
        else
        {
          switch (cur_state_)
          {
          case ROW_NOT_FOUND:
            /**
             * there is only one cell in row index for this row, and we only
             * return one obj with extend field(OP_ROW_DOES_NOT_EXIST)  
             */
            obj.set_ext(ObActionFlag::OP_ROW_DOES_NOT_EXIST);
            null_cell_info_.column_id_ = 0;
            null_cell_info_.table_id_ = cell->table_id_;
            null_cell_info_.row_key_ = cell->row_key_;
            null_cell_info_.value_ = obj;
            *cell_info = &null_cell_info_;
            cur_state_ = GET_NEXT_ROW;
            break;
          case ITERATE_NORMAL:
            ret = getter_.get_cell(cell_info);
            if (OB_SUCCESS == ret)
            {
              (*cell_info)->table_id_ = cell->table_id_;
            }
            break;
          case GET_NEXT_ROW:
            TBSYS_LOG(WARN, "invalid internal state(GET_NEXT_ROW) in get_cell()");
          case ITERATE_END:
          default:
            null_cell_info_.reset();
            *cell_info = &null_cell_info_;
            break;
          }
        }
      }

      if (OB_SUCCESS == ret)
      {
        if (prev_reader_idx_ != cur_reader_idx_)
        {
          if (NULL != is_row_changed)
          {
            *is_row_changed = true;
          }
          prev_reader_idx_ = cur_reader_idx_;
        }
        else
        {
          if (NULL != is_row_changed)
          {
            *is_row_changed = false;
          }
        }
      }

      return ret;
    }

    void ObSSTableGetter::reset()
    {
      cur_state_ = ITERATE_NORMAL;

      readers_ = NULL;
      readers_size_ = 0;
      cur_reader_idx_ = 0;
      prev_reader_idx_ = -1;

      handled_cells_ = 0;
      column_group_num_ = 0;
      cur_column_group_idx_ = 0;

      get_param_ = NULL;
      cur_column_mask_.reset();
      null_cell_info_.reset();

      block_cache_ = NULL;
      block_index_cache_ = NULL;
    }

    int ObSSTableGetter::init(ObBlockCache& block_cache, 
                              ObBlockIndexCache& block_index_cache, 
                              const common::ObGetParam& get_param, 
                              const ObSSTableReader* const readers[], 
                              const int64_t reader_size)
    {
      int ret = OB_SUCCESS;
      inited_ = false;

      if (NULL == readers || reader_size <= 0)
      {
        TBSYS_LOG(WARN, "invalid param, readers=%p, size=%ld", readers, reader_size);
        ret = OB_INVALID_ARGUMENT;
      }
      else if (NULL == get_param.get_row_index() || get_param.get_row_size() <= 0)
      {
        TBSYS_LOG(WARN, "invalid get param, row_index=%p, row_size=%ld",
                  get_param.get_row_index(), get_param.get_row_size());
        ret = OB_ERROR;
      }
      else 
      {
        reset();
        readers_ = readers;
        readers_size_ = reader_size;
        get_param_ = &get_param;
        block_cache_ = &block_cache;
        block_index_cache_ = &block_index_cache;

        inited_ = true;
        ret = filter_column_group();
        if (OB_SUCCESS == ret)
        {
          ret = load_cur_block();
          if (OB_SEARCH_NOT_FOUND == ret)
          {
            cur_state_ = ROW_NOT_FOUND;
            ret = OB_SUCCESS;
          }
          else if (OB_SUCCESS != ret)
          {
            TBSYS_LOG(WARN, "failed to load current block, ret=%d", ret);
          }
        }
        else if (OB_SEARCH_NOT_FOUND == ret)
        {
          cur_state_ = ROW_NOT_FOUND;
          ret = OB_SUCCESS;
        }
      }

      return ret;
    }

    int ObSSTableGetter::load_cur_block()
    {
      int ret                 = OB_SUCCESS;
      SearchMode mode         = OB_SEARCH_MODE_GREATER_EQUAL;
      const ObCellInfo* cell  = NULL;
      uint64_t table_id       = OB_INVALID_ID;
      ObString look_key;
      ObBlockIndexPositionInfo info;

      if (NULL == readers_[cur_reader_idx_])
      {
        ret = OB_SEARCH_NOT_FOUND;
      }

      if (OB_SUCCESS == ret)
      {
        cell = get_cur_param_cell();
        if (NULL == cell)
        {
          TBSYS_LOG(WARN, "crrent cell in get param is NULL");
          ret = OB_ERROR;
        }
        else
        {
          look_key = cell->row_key_;
          table_id = cell->table_id_;
        }
      }

      if (OB_SUCCESS == ret)
      {
        const ObSSTableTrailer& trailer = readers_[cur_reader_idx_]->get_trailer();
  
        memset(&info, 0, sizeof(info));
        info.sstable_file_id_ = readers_[cur_reader_idx_]->get_sstable_id().sstable_file_id_;
        info.offset_ = trailer.get_block_index_record_offset();
        info.size_   = trailer.get_block_index_record_size();
     
        ret = block_index_cache_->get_single_block_pos_info(info, table_id, 
                                                            column_group_[cur_column_group_idx_],
                                                            look_key, mode, block_pos_); 
        if (OB_SUCCESS == ret)  //load block index success
        {
          ret = init_column_mask();
          if (OB_SUCCESS == ret)
          {
            ret = fetch_block();
            if (OB_SUCCESS != ret && OB_SEARCH_NOT_FOUND != ret)
            {
              TBSYS_LOG(WARN, "fetch_block error, ret=%d", ret);
            }
          }
          else
          {
            ret = OB_SEARCH_NOT_FOUND;
          }
        }
        else if (ret == OB_BEYOND_THE_RANGE)
        {
          TBSYS_LOG(DEBUG, "Not find block for rowkey:");
          hex_dump(look_key.ptr(), look_key.length(), true);
          ret = OB_SEARCH_NOT_FOUND;
        }
        else
        {
          TBSYS_LOG(WARN, "failed to apply block index, ret=%d", ret);
        }
      }
      
      return ret;
    }

    int ObSSTableGetter::init_column_mask()
    {
      int ret               = OB_SUCCESS;
      int32_t index         = -1;
      uint64_t table_id     = OB_INVALID_ID;
      uint64_t column_id    = OB_INVALID_ID;
      int64_t column_count  = 0;
      int64_t cell_size     = 0;
      int64_t start         = 0;
      int64_t end           = 0;
      const ObSSTableReader* reader               = NULL;
      const ObSSTableSchema* schema               = NULL;
      const ObSSTableSchemaColumnDef* column_def  = NULL;
      const ObGetParam::ObRowIndex* row_index     = NULL;
      
      /**
       * there is problem that the get param only includes unique cell 
       * in one row, if user want to get the same cell in the row more 
       * than once, we only return the column once. for example: if 
       * get column 1 2 1, we only return column 1 2. merger server 
       * can solove this problem 
       */
      cell_size     = get_param_->get_cell_size();
      row_index     = get_param_->get_row_index();
      start         = row_index[cur_reader_idx_].offset_;
      end           = row_index[cur_reader_idx_].offset_
                      + row_index[cur_reader_idx_].size_;

      if (start >= cell_size || end > cell_size)
      {
        TBSYS_LOG(WARN, "wrong row range, cell_size=%ld, start=%ld, end=%ld",
                  cell_size, start, end);
        ret = OB_ERROR;
      }

      if (OB_SUCCESS == ret)
      {
        reader = readers_[cur_reader_idx_];
        if (NULL != reader)
        {
          table_id = (*get_param_)[start]->table_id_;
          schema = reader->get_schema();
          if (NULL == schema)
          {
            TBSYS_LOG(WARN, "schema is NULL, cur_reader_idx_=%d, reader=%p", 
                      cur_reader_idx_, reader);
            ret = OB_ERROR;
          }
        }
        else
        {
          TBSYS_LOG(WARN, "reader is NULL, cur_reader_idx_=%d", cur_reader_idx_);
          ret = OB_ERROR;
        }
      }

      if (OB_SUCCESS == ret)
      {
        //reset current column mask, clear the mask before
        cur_column_mask_.reset();
        
        for (int64_t i = start; i < end && OB_SUCCESS == ret; ++i)
        {
          column_id = (*get_param_)[i]->column_id_;
          if (OB_FULL_ROW_COLUMN_ID == column_id)
          {
            //column id equal to 0, it means to get all the columns in row
            column_def = schema->get_group_schema(table_id, 
                                                  column_group_[cur_column_group_idx_], 
                                                  column_count);
            if (NULL != column_def && column_count > 0)
            {
              for (int j = 0; j < column_count && OB_SUCCESS == ret; j++)
              {
                ret = cur_column_mask_.add_column_id(j, column_def[j].column_name_id_);
              }
            }
            else
            {
              ret = OB_ERROR;
            }
            break;
          }
          else if (!schema->is_column_exist(table_id, column_id))
          {
            // column id not exist in schema, set to NOT_EXIST_COLUMN
            // return NullType .
            if (0 == cur_column_group_idx_)
            {
              ret = cur_column_mask_.add_column_id(
                  ObScanColumnIndexes::NOT_EXIST_COLUMN, column_id);
            }
          }
          else
          {
            index = schema->find_offset_column_group_schema(
              table_id, column_group_[cur_column_group_idx_], column_id);
            if (index < 0)
            {
              /**
               * not found column id, the column doesn't belong to current 
               * column group,do nothing. 
               */
            }
            else
            {
              ret = cur_column_mask_.add_column_id(index, column_id);   
            }
          }
        }
      }

      return ret;
    }

    int ObSSTableGetter::get_block_data(const uint64_t sstable_id, const uint64_t table_id, 
                                        const char*& block_data, int64_t& block_data_size)
    {
      int ret                   = OB_SUCCESS;
      int64_t real_size         = 0;
      const char* comp_buf      = NULL;
      int64_t comp_data_size    = 0;
      ObCompressor* compressor  = NULL;
      ObBufferHandle handler;
      ObRecordHeader header;
      
      block_data = NULL;
      block_data_size = 0;

      if (OB_SUCCESS == ret)
      {
        ret = block_cache_->get_block(sstable_id, block_pos_.offset_, block_pos_.size_, 
                                      handler, table_id);
        if (ret <= 0)
        {
          TBSYS_LOG(WARN, "load block from obcache failed, ret=%d", ret);
          ret = OB_ERROR;
        }
        else 
        {
          ret = OB_SUCCESS;
        }
      }

      if (OB_SUCCESS == ret)
      {
        ret = ObRecordHeader::check_record(handler.get_buffer(), block_pos_.size_, 
                                           ObSSTableWriter::DATA_BLOCK_MAGIC, header, 
                                           comp_buf, comp_data_size);
        if (OB_SUCCESS != ret)
        {
          TBSYS_LOG(WARN, "invalid block data, block_size=%ld, sstable_id=%lu, table_id=%lu",
                    block_pos_.size_, sstable_id, table_id);
        }
      }

      if (OB_SUCCESS == ret)
      {
        ret = uncomp_buf_.ensure_space(header.data_length_, ObModIds::OB_SSTABLE_EGT_SCAN);
      }

      if (OB_SUCCESS == ret)
      {
        if (header.is_compress())
        {
          compressor = 
            const_cast<ObSSTableReader*>(readers_[cur_reader_idx_])->get_decompressor();
          if (NULL != compressor)
          {
            ret = compressor->decompress(comp_buf, comp_data_size, uncomp_buf_.get_buffer(),
                                         uncomp_buf_.get_buffer_size(), real_size);
            if (OB_SUCCESS != ret)
            {
              TBSYS_LOG(WARN, "decompress failed, data_length=%d data_zlength=%d",
                        header.data_length_, header.data_zlength_);
            }
            else
            {
              if (real_size != header.data_length_)
              {
                TBSYS_LOG(WARN, "the uncompressed length=%ld, the expected length=%d",
                          real_size, header.data_length_);
                ret = OB_ERROR;
              }
              else
              {
                block_data = uncomp_buf_.get_buffer();
                block_data_size = real_size;
              }
            }
          }
          else
          {
            TBSYS_LOG(WARN, "failed to get compressor");
            ret = OB_ERROR;
          }
        }
        else
        {
          memcpy(uncomp_buf_.get_buffer(), comp_buf, comp_data_size);
          block_data = uncomp_buf_.get_buffer();
          block_data_size = comp_data_size;
        }
      }

      return ret;
    }

    int ObSSTableGetter::fetch_block() 
    {
      int ret                       = OB_SUCCESS;
      const ObCellInfo* cell        = NULL;
      int64_t data_size             = 0;
      const char *data_buf          = NULL;
      const ObSSTableReader* reader = NULL;
      int64_t store_style           = OB_SSTABLE_STORE_DENSE;
      
      cell = get_cur_param_cell();
      if (NULL == cell)
      {
        TBSYS_LOG(WARN, "crrent cell in get param is NULL");
        ret = OB_ERROR;
      }

      if (OB_SUCCESS == ret)
      {
        reader = readers_[cur_reader_idx_];
        if (NULL == reader)
        {
          TBSYS_LOG(WARN, "reader is NULL, cur_reader_idx_=%d", cur_reader_idx_);
          ret = OB_ERROR;
        }
      }

      if (OB_SUCCESS == ret)
      {
        ret = get_block_data(reader->get_sstable_id().sstable_file_id_, 
                             cell->table_id_, data_buf, data_size);
      }

      if (OB_SUCCESS == ret)
      {
        store_style = reader->get_trailer().get_row_value_store_style();
        ret = getter_.init(cell->row_key_, data_buf, data_size, store_style);
        if (OB_SUCCESS != ret && OB_SEARCH_NOT_FOUND != ret)
        {
          TBSYS_LOG(WARN, "block getter initialize error, ret=%d", ret);  
        }
      }

      return ret;
    }

    bool ObSSTableGetter::is_column_group_id_existent(const uint64_t column_group_id)
    {
      bool bret = false;

      if (column_group_num_ > 0)
      {
        for (int64_t i = 0; i < column_group_num_; ++i)
        {
          if (column_group_id == column_group_[i])
          {
            bret = true;
            break;
          }
        }
      }

      return bret;
    }

    int ObSSTableGetter::add_column_group_id(const ObSSTableSchema& schema, 
                                             const uint64_t table_id,
                                             const uint64_t column_id)
    {
      int ret                   = OB_SUCCESS;
      uint64_t column_group_id  = OB_INVALID_ID;
      int64_t column_index      = -1;

      /**
       * FIXME:currently we don't handle one column belongs to 
       * multiple column group, we only use the first column group the 
       * column belongs to. 
       */
      column_index = schema.find_offset_first_column_group_schema(
        table_id, column_id, column_group_id);
      if (column_index < 0 || OB_INVALID_ID == column_group_id)
      {
        //column is non-existent in schema
        TBSYS_LOG(DEBUG, "can't find column group id from sstable schema, "
                          "table_id=%lu, column_id=%lu", 
                  table_id, column_id);
      }
      else if (!is_column_group_id_existent(column_group_id))
      {
        column_group_[column_group_num_++] = column_group_id;
      }

      return ret;
    }

    int ObSSTableGetter::filter_column_group()
    {
      int ret                   = OB_SUCCESS;
      uint64_t table_id         = OB_INVALID_ID;
      uint64_t column_id        = OB_INVALID_ID;
      int64_t cell_size         = 0;
      int64_t start             = 0;
      int64_t end               = 0;
      int64_t column_group_size = OB_MAX_COLUMN_GROUP_NUMBER;
      const ObSSTableReader* reader           = NULL;
      const ObSSTableSchema* schema           = NULL;
      const ObGetParam::ObRowIndex* row_index = NULL;

      cell_size     = get_param_->get_cell_size();
      row_index     = get_param_->get_row_index();
      start         = row_index[cur_reader_idx_].offset_;
      end           = row_index[cur_reader_idx_].offset_
                      + row_index[cur_reader_idx_].size_;

      if (start >= cell_size || end > cell_size)
      {
        TBSYS_LOG(WARN, "wrong row range, cell_size=%ld, start=%ld, end=%ld",
                  cell_size, start, end);
        ret = OB_ERROR;
      }

      if (OB_SUCCESS == ret)
      {
        reader = readers_[cur_reader_idx_];
        if (NULL != reader)
        {
          table_id = (*get_param_)[start]->table_id_;
          schema = reader->get_schema();
          if (NULL == schema)
          {
            TBSYS_LOG(WARN, "schema is NULL, cur_reader_idx_=%d, "
                            "reader=%p, table_id=%lu", 
                      cur_reader_idx_, reader, table_id);
            ret = OB_ERROR;
          }
        }
        else
        {
          ret = OB_SEARCH_NOT_FOUND;
        }
      }

      if (OB_SUCCESS == ret)
      {
        //reset current column group count and index
        column_group_num_ = 0;
        cur_column_group_idx_ = 0;

        for (int64_t i = start; i < end && OB_SUCCESS == ret; ++i)
        {
          column_id = (*get_param_)[i]->column_id_;
          if (OB_FULL_ROW_COLUMN_ID == column_id)
          {
            //column id equal to 0, it means to get all the columns in row
            ret = schema->get_table_column_groups_id(table_id, column_group_, 
                                                     column_group_size);
            if (OB_SUCCESS == ret && column_group_size > 0)
            {
              if (row_index[cur_reader_idx_].size_ > 1)
              {
                TBSYS_LOG(WARN, "more than one cell with same rowkey and "
                                "one cell's column id is 0, just return the "
                                "whole row instead");
              }
              //change the column size of this row to get
              column_group_num_ = column_group_size;
            }
            else if (column_group_size <= 0)
            {
              ret = OB_ERROR;
            }
            break;
          }
          else
          {
            ret = add_column_group_id(*schema, table_id, column_id);
          }
        }
      }

      if (OB_SUCCESS == ret && 0 == column_group_num_)
      {
        /**
         * we must ensure there is one column group at least, if there 
         * is not column group, we use the column group id in the first 
         * column default. 
         */
        column_group_num_ = 1;
        column_group_[0] = schema->get_column_def(0)->column_group_id_;
      }

      return ret;
    }
  }//end namespace sstable
}//end namespace oceanbase
