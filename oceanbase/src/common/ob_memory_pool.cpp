/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_memory_pool.cpp for ...
 *
 * Authors:
 *   wushi <wushi.ly@taobao.com>
 *
 */
#include "ob_memory_pool.h"
#include <new>
#include <malloc.h>
#include <errno.h>
#include <execinfo.h>
#include <numa.h>
#include "ob_link.h"
#include "tbsys.h"
#include "tblog.h"

using namespace oceanbase::common;
namespace 
{
  /// @brief  如果设置该环境变量则直接调用系统malloc和free分配释放内存，
  ///         目的是便于内存bug调试因为对于内存池缓存的内存，valgrind没有办法检查
  const char * OB_MALLOC_DIRECT_ENV_NAME = "__OB_MALLOC_DIRECT__";

  /// @brief magic number, used for memory overwrite debug
  const uint32_t OB_MEMPOOL_MAGIC=0x71d9f5e7;



  /// @struct  MemBlockInfo identify a block allocated from system
  struct MemBlockInfo
  {
    /// @property magic number
    uint32_t  magic_;
    /// @property reference number
    int32_t   ref_num_; 
    /// @property link all memblockinfo into list
    oceanbase::common::ObDLink     block_link_;
    /// @property size of block, include all memory allocated from system
    int64_t   block_size_; 
    /// @property buffer adress
    char      buf_[0];
  };

  /// @fn init MemBlockInfo
  void init_mem_block_info(MemBlockInfo &info, 
                           const int64_t block_size, const int32_t ref_num)
  {
    oceanbase::common::ObDLink *link = NULL;
    link = new(&(info.block_link_))oceanbase::common::ObDLink;
    info.block_size_ = block_size;
    info.magic_ = OB_MEMPOOL_MAGIC;
    info.ref_num_ = ref_num;
  }

  /// @fn check if mem block info is correct, return true if check passed
  bool check_mem_block_info(const MemBlockInfo & info)
  {
    return(info.magic_ == OB_MEMPOOL_MAGIC);
  }

  /// @struct  MemPoolItemInfo identify a item allocated from mempool
  struct MemPoolItemInfo
  {
    /// @property magic number
    uint32_t        magic_;
    /// @property   which module allocated this memory item
    int32_t         mod_id_;
    /// @property identify the memory was allocated from which MemBlockInfo
    MemBlockInfo    *mother_block_;
    /// @property buffer adress
    char      buf_[0];
  };

  /// @fn init mem pool item info
  void init_mem_pool_item_info(MemPoolItemInfo & info, MemBlockInfo *mother_block,
                               int32_t mod_id = 0)
  {
    info.mother_block_ = mother_block;
    info.mod_id_ = mod_id;
    info.magic_ = OB_MEMPOOL_MAGIC;
  }

  /// @fn check if mem pool item info is correct, return true if check passed
  bool check_mem_pool_item_info(const MemPoolItemInfo &info)
  {
    return(info.magic_ == OB_MEMPOOL_MAGIC 
           && info.mother_block_ != NULL
           && check_mem_block_info(*info.mother_block_));
  }

  /// @fn allocate a new block
  void * allocate_new_block(MemBlockInfo * & block_info, MemPoolItemInfo * & item_info,
                            const int64_t block_size, int32_t mod_id, bool numa_enabled)
  {
    char *result = NULL;
    if (numa_enabled)
    {
      result = static_cast<char*>(numa_alloc_interleaved(block_size));
    }
    else
    {
      result = new(std::nothrow) char[block_size];
    }
    if (result != NULL)
    {
      block_info = reinterpret_cast<MemBlockInfo *>(result);
      item_info = reinterpret_cast<MemPoolItemInfo*>(block_info->buf_);
      init_mem_block_info(*block_info, block_size, 1);
      init_mem_pool_item_info(*item_info, block_info,mod_id);
      result = item_info->buf_;
    }
    return result;
  }

  /// @brief minimum number of bytes allocated from system with each alloc, 64k
  const int64_t OB_MINIMUM_MALLOC_MEM_SIZE = 64*1024 + sizeof(MemBlockInfo) 
                                             + sizeof(MemPoolItemInfo);
  /// @property default fixed item size
  const int64_t OBFIXEDMEMPOOL_DEFAULT_ITEM_SIZE = OB_MINIMUM_MALLOC_MEM_SIZE - 
                                                   sizeof(MemPoolItemInfo) - sizeof(MemBlockInfo);

  /// @property default item number each block of fixed memory pool
  const int64_t OBFIXEDMEMPOOL_DEFAULT_ITEM_NUM_EACHE_BLOCK = 1;


  /// @fn check if should call malloc and free directly
  bool malloc_directly()
  {
    return getenv(OB_MALLOC_DIRECT_ENV_NAME) != NULL;
  }  
}

oceanbase::common::ObBaseMemPool::ObBaseMemPool()
:mem_size_handled_(0), mem_size_limit_(INT64_MAX),mem_size_default_mod_(0),mem_size_each_mod_(NULL),mod_set_(NULL),numa_enabled_(false)
{
}

oceanbase::common::ObBaseMemPool::~ObBaseMemPool()
{
  delete [] mem_size_each_mod_;
  mem_size_each_mod_ = NULL;
}

int64_t oceanbase::common::ObBaseMemPool::shrink(const int64_t )
{
  return 0;
}


int64_t oceanbase::common::ObBaseMemPool::get_mod_memory_usage(int32_t mod_id)
{
  int64_t result = 0;
  if ((NULL == mod_set_)  || (NULL == mem_size_each_mod_))
  {
    result = mem_size_default_mod_;
  }
  else
  {
    if (mod_id < 0 || mod_id >= mod_set_->get_max_mod_num())
    {
      result = mem_size_each_mod_[0];
    }
    else
    {
      result = mem_size_each_mod_[mod_id];
    }
  }
  return result;
}

void oceanbase::common::ObBaseMemPool::print_mod_memory_usage(bool print_to_std)
{
  TBSYS_LOG(INFO, "total_mem_size [%ld]", mem_size_handled_);
  malloc_stats();
  if ((NULL == mod_set_)  || (NULL == mem_size_each_mod_))
  {
    TBSYS_LOG(INFO, "total used memory size:%ld", mem_size_default_mod_);
    if (print_to_std)
    {
      fprintf(stderr, "total used memory size:%ld\n", static_cast<int64_t>(mem_size_default_mod_));
    }
  }
  else
  {
    for (int32_t mod_idx = 0; mod_idx < mod_set_->get_max_mod_num(); mod_idx ++)
    {
      if (mod_set_->get_mod_name(mod_idx) != NULL)
      {
        TBSYS_LOG(INFO, "module size static [mod:%s,size:%ld]", mod_set_->get_mod_name(mod_idx),
                  mem_size_each_mod_[mod_idx]);
        if (print_to_std)
        {
          fprintf(stderr, "module size static [mod:%s,size:%ld]\n", mod_set_->get_mod_name(mod_idx),
                  static_cast<int64_t>(mem_size_each_mod_[mod_idx]));
        }
      }
    }
  }
  TBSYS_LOG(INFO, "module size static [memory_size_handled:%ld,"
            "direct_allocated_block_num_:%ld,direct_allocated_mem_size_:%ld]",  
            mem_size_handled_,direct_allocated_block_num_,direct_allocated_mem_size_);
}

void oceanbase::common::ObBaseMemPool::mod_malloc(const int64_t size, const int32_t mod_id)
{
  int32_t real_mod_id = mod_id;
  if ((NULL == mod_set_)  || (NULL == mem_size_each_mod_) )
  {
    mem_size_default_mod_ += size;
  }
  else
  {
    if (mod_id <= 0 || mod_id >= mod_set_->get_max_mod_num())
    {
      real_mod_id = 0;
    }
    mem_size_each_mod_[real_mod_id] += size;
  }
}

void oceanbase::common::ObBaseMemPool::mod_free(const int64_t size, const int32_t mod_id)
{
  int32_t real_mod_id = mod_id;
  if ((NULL == mod_set_)  || (NULL == mem_size_each_mod_) )
  {
    mem_size_default_mod_ -= size;
  }
  else
  {
    if (mod_id <= 0 || mod_id >= mod_set_->get_max_mod_num())
    {
      real_mod_id = 0;
    }
    mem_size_each_mod_[real_mod_id] -= size;
  }
}

int oceanbase::common::ObBaseMemPool::init(const ObMemPoolModSet * mod_set, bool numa_enabled/* = false*/)
{
  int err = OB_SUCCESS;
  if (NULL != mod_set && mod_set->get_max_mod_num() > 0)
  {
    mem_size_each_mod_ = new int64_t[mod_set->get_max_mod_num()];
    if (NULL == mem_size_each_mod_)
    {
      err = OB_ALLOCATE_MEMORY_FAILED;
    }
    else
    {
      memset(mem_size_each_mod_, 0x00, sizeof(int64_t)*mod_set->get_max_mod_num());
    }
  }
  if (numa_enabled)
  {
    err = numa_available();
    if (-1 == err)
    {
      TBSYS_LOG(ERROR, "numa_available invoke error");
      err = OB_ERROR;
    }
  }
  if (OB_SUCCESS == err)
  {
    mod_set_ = mod_set;
    numa_enabled_ = numa_enabled;
  }
  else
  {
    TBSYS_LOG(ERROR, "ObBaseMemPool::init error, err=%d mod_set=%p numa_enabled=%s",
              err, mod_set, numa_enabled?"true":"false");
  }
  return err;
}

int64_t oceanbase::common::ObBaseMemPool::get_memory_size_handled()
{
  tbsys::CThreadGuard guard(&pool_mutex_);
  return mem_size_handled_;
}

int64_t oceanbase::common::ObBaseMemPool::set_memory_size_limit(const int64_t mem_size_limit)
{
  if (0 < mem_size_limit)
  {
    mem_size_limit_ = mem_size_limit;
  }
  return mem_size_limit_;
}

int64_t oceanbase::common::ObBaseMemPool::get_memory_size_limit()
{
  return mem_size_limit_;
}


void *oceanbase::common::ObBaseMemPool::malloc_emergency(const int64_t nbyte,
                                                         const int32_t mod_id, int64_t *got_size )
{
  return malloc_(nbyte,mod_id, got_size);
}

void *oceanbase::common::ObBaseMemPool::malloc(const int64_t nbyte, int32_t mod_id, int64_t *got_size)
{
  void *ret = NULL;
  if (mem_size_handled_ > mem_size_limit_)
  {
    TBSYS_LOG(ERROR, "memory over limited handled=%ld limit=%ld",
              mem_size_handled_, mem_size_limit_);
    errno = ENOMEM;
  }
  else
  {
    ret = malloc_(nbyte,mod_id, got_size);
    //TBSYS_LOG(INFO, "ob_malloc ptr=%p size=%ld", ret, nbyte);
  }
  return ret;
}

void oceanbase::common::ObBaseMemPool::free(const void *ptr)
{
  //TBSYS_LOG(INFO, "ob_free ptr=%p", ptr);
  free_(ptr);
}




void oceanbase::common::ObFixedMemPool::property_initializer()
{
  oceanbase::common::ObDLink *link = NULL;
  link = new(&used_mem_block_list_)oceanbase::common::ObDLink;
  link = new(&free_mem_block_list_)oceanbase::common::ObDLink;
  mem_block_size_ = 0;
  mem_fixed_item_size_ = 0;
  used_mem_block_num_ = 0;
  free_mem_block_num_ = 0;
  direct_allocated_block_num_ = 0;
  direct_allocated_mem_size_ = 0;
}

oceanbase::common::ObFixedMemPool::ObFixedMemPool()
{
  property_initializer();
}


int oceanbase::common::ObFixedMemPool::init(const int64_t fixed_item_size,  
                                            const int64_t item_num_each_block, 
                                            const ObMemPoolModSet *mod_set/* = NULL*/,
                                            const bool numa_enabled/* = false*/)
{
  int err = 0;
  int64_t real_fixed_item_size = fixed_item_size;
  int64_t real_item_num_each_block = item_num_each_block;
  if (real_fixed_item_size <= 0 || real_item_num_each_block <= 0)
  {
    TBSYS_LOG(WARN, "param invalid [fixed_item_size:%ld, item_num_each_block:%ld]",
              fixed_item_size, item_num_each_block);
    err = EINVAL;
  }
  tbsys::CThreadGuard guard(&pool_mutex_);
  if (mem_block_size_ > 0)
  {
    TBSYS_LOG(WARN, "double initializing memory pool");
    err = EINVAL;
  }
  if ( 0 == err )
  {
    err = ObBaseMemPool::init(mod_set, numa_enabled);
  }
  if (0 == err)
  {
    int64_t real_block_size = (real_fixed_item_size+sizeof(MemPoolItemInfo))*item_num_each_block
                              + sizeof(MemBlockInfo);
    mem_fixed_item_size_ = real_fixed_item_size;
    if (real_block_size <= OB_MINIMUM_MALLOC_MEM_SIZE)
    {
      mem_block_size_ = OB_MINIMUM_MALLOC_MEM_SIZE;
    }
    else
    {
      mem_block_size_ = real_block_size;
    }
  }

  errno = err;
  return(-1*errno);
}


oceanbase::common::ObFixedMemPool::~ObFixedMemPool()
{
  clear(true);
}

int64_t oceanbase::common::ObFixedMemPool::get_block_num() const
{
  int64_t result = 0;
  tbsys::CThreadGuard guard(&pool_mutex_);
  if (mem_block_size_ <= 0)
  {
    TBSYS_LOG(WARN,"memory pool not initialized");
  }
  else
  {
    result = used_mem_block_num_ + free_mem_block_num_;
  }
  return  result;
}

int64_t oceanbase::common::ObFixedMemPool::get_used_block_num() const
{
  int64_t result = 0;
  tbsys::CThreadGuard guard(&pool_mutex_);
  if (mem_block_size_ <= 0)
  {
    TBSYS_LOG(WARN,"memory pool not initialized");
  }
  else
  {
    result = used_mem_block_num_; 
  }
  return  result;
}


void oceanbase::common::ObFixedMemPool::print_mod_memory_usage(bool print_to_std)
{
  ObBaseMemPool::print_mod_memory_usage(print_to_std);
  TBSYS_LOG(INFO, "module size static [used_mem_block_num_:%ld,free_mem_block_num_:%ld,"
            "mem_block_size_:%ld]", 
            used_mem_block_num_, free_mem_block_num_, mem_block_size_); 
  if (print_to_std)
  {
    fprintf(stderr, "module size static [used_mem_block_num_:%ld,free_mem_block_num_:%ld,"
            "mem_block_size_:%ld]\n", 
            static_cast<int64_t>(used_mem_block_num_), 
            static_cast<int64_t>(free_mem_block_num_), 
            static_cast<int64_t>(mem_block_size_)); 
  }
}


int64_t oceanbase::common::ObFixedMemPool::recycle_memory_block(ObDLink * &block_it, 
                                                                bool check_unfreed_mem)
{
  int64_t memory_freed = 0;
  char *buf_allocated = NULL;
  MemBlockInfo *info = CONTAINING_RECORD(block_it, MemBlockInfo, block_link_);
  mem_size_handled_ -= info->block_size_;
  memory_freed += info->block_size_;
  block_it = info->block_link_.next();
  info->block_link_.remove();
  buf_allocated = reinterpret_cast<char*>(info);
  if (info->ref_num_ != 0 )
  {
    used_mem_block_num_ --;
    if (check_unfreed_mem)
    {
      /*
      TBSYS_LOG(ERROR, "memory not released [begin_addr:%p,"
                "end_addr:%u]",buf_allocated, buf_allocated + info->block_size_);
                */
    }
  }
  else
  {
    free_mem_block_num_ --;
  }

  if (numa_enabled_)
  {
    numa_free(buf_allocated, info->block_size_);
  }
  else
  {
    delete [] buf_allocated;
  }
  buf_allocated = NULL;
  return memory_freed;
}

void oceanbase::common::ObFixedMemPool::clear()
{
  clear(false);
}

void oceanbase::common::ObFixedMemPool::clear(bool check_unfreed_mem)
{
  tbsys::CThreadGuard guard(&pool_mutex_);
  ObDLink *block_it = free_mem_block_list_.next();
  while (block_it != &free_mem_block_list_)
  {
    recycle_memory_block(block_it, check_unfreed_mem);
  }
  block_it = used_mem_block_list_.next();
  while (block_it != & used_mem_block_list_)
  {
    recycle_memory_block(block_it, check_unfreed_mem);
  }
  property_initializer();
}

void * oceanbase::common::ObFixedMemPool::malloc_(const int64_t nbyte,const int32_t mod_id, int64_t *got_size)
{
  /// @todo (wushi wushi.ly@taobao.com) 现在任何内存分配都是直接给一个整块，
  /// 但是对于链表和hash表这种东西里面会有小块内存分配，
  /// 必须对这种情况进行优化，否则太浪费内存了，在2010.08.31之前完成
  char *result = NULL;
  int result_errno = 0;
  MemBlockInfo *block_info = NULL;
  MemPoolItemInfo *item_info = NULL;
  int64_t size_malloc = 0;
  if (nbyte <= 0)
  {
    TBSYS_LOG(WARN, "cann't allocate memory less than 0 byte [nbyte:%ld]", nbyte);
    result_errno = EINVAL;
  }
  int64_t block_size = nbyte+sizeof(MemBlockInfo) + sizeof(MemPoolItemInfo);

  if (malloc_directly())
  {
    result = new(std::nothrow) char[nbyte];
    if (NULL == result)
    {
      result_errno = errno;
    }
  }

  if (NULL == result && mem_block_size_ <= 0)
  {
    TBSYS_LOG(WARN, "memory pool not initialized");
    result_errno = EINVAL;
  }
  /// allocated from system
  if (NULL == result && result_errno == 0 && nbyte > mem_fixed_item_size_)
  {
    result =reinterpret_cast<char*>(allocate_new_block(block_info,item_info,block_size,mod_id,numa_enabled_));
    if (result == NULL)
    {
      result_errno = errno;
    }
    else
    {
      tbsys::CThreadGuard guard(&pool_mutex_);
      used_mem_block_list_.insert_next(block_info->block_link_);
      /// statistic 
      mem_size_handled_ += block_info->block_size_;
      used_mem_block_num_ ++;
      direct_allocated_block_num_ ++;
      direct_allocated_mem_size_ += block_info->block_size_;
      size_malloc = block_info->block_size_;
    }
    if (block_size  < mem_block_size_)
    {
      TBSYS_LOG(ERROR, "try to allocate variable sized memory from fixed memory pool "
                "[nbyte:%ld,mem_fixed_item_size_:%ld]", nbyte, mem_fixed_item_size_);
    }
  }

  {
    tbsys::CThreadGuard guard(&pool_mutex_);
    /// allocate from free block list
    if (result_errno == 0 
        && result == NULL
        && !free_mem_block_list_.is_empty())
    {
      block_info = CONTAINING_RECORD(free_mem_block_list_.next(), MemBlockInfo,block_link_);
      item_info = reinterpret_cast<MemPoolItemInfo *>(block_info->buf_);
      block_info->block_link_.remove();
      used_mem_block_list_.insert_next(block_info->block_link_);
      init_mem_pool_item_info(*item_info,block_info);
      result = item_info->buf_;
      /// statistic
      block_info->ref_num_ ++;
      used_mem_block_num_ ++;
      free_mem_block_num_ --;
      size_malloc = block_info->block_size_;
      item_info->mod_id_ = mod_id;
    }
  }

  /// allocated from system
  if (result_errno == 0 && result == NULL)
  {
    result = reinterpret_cast<char*>(allocate_new_block(block_info,
                                                        item_info,mem_block_size_,mod_id,numa_enabled_));
    if (result == NULL)
    {
      result_errno = errno;
    }
    else
    {
      tbsys::CThreadGuard guard(&pool_mutex_);
      used_mem_block_list_.insert_next(block_info->block_link_);
      /// statistic
      used_mem_block_num_ ++;
      result = item_info->buf_;
      mem_size_handled_ += block_info->block_size_;
      size_malloc = block_info->block_size_;
    }
  }
  if (0 == result_errno)
  {
    if (NULL != got_size)
    {
      *got_size = size_malloc - static_cast<int64_t>(sizeof(MemBlockInfo) + sizeof(MemPoolItemInfo));
    }
    tbsys::CThreadGuard guard(&pool_mutex_);
    mod_malloc(size_malloc,mod_id);
  }
  errno = result_errno;
  return result;
}


void  oceanbase::common::ObFixedMemPool::free_(const void *ptr)
{
  int result_errno = 0;
  int64_t size_free = 0;
  int32_t mod_id = 0;
  bool freed = false;
  bool need_free = false;
  const MemPoolItemInfo *item_info = NULL;
  MemBlockInfo *block_info = NULL;
  const char *buf = reinterpret_cast<const char*>(ptr);
  if (ptr == NULL)
  {
    freed = true;
  }
  if (malloc_directly())
  {
    delete [] reinterpret_cast<const char*>(ptr);
    freed = true;
  }
  {
    tbsys::CThreadGuard guard(&pool_mutex_);
    if (mem_block_size_ <= 0)
    {
      //TBSYS_LOG(WARN, "memory pool not initialized");
      result_errno = EINVAL;
    }
    if (0 == result_errno && !freed)
    {
      buf -= sizeof(MemPoolItemInfo);
      item_info = reinterpret_cast<const MemPoolItemInfo*>(buf);
      block_info = item_info->mother_block_;
      if (!check_mem_pool_item_info(*item_info))
      {
        TBSYS_LOG(ERROR, "memory corrupt [addr:%p,"
                  "MemPoolItemInfo::magic_:%u,"
                  "MemPoolItemInfo::mother_block_:%p,"
                  "MemBlockInfo::magic_:%u,"
                  "correct magic number:%u]",ptr, item_info->magic_,
                  block_info,
                  ((block_info != NULL) ? (block_info->magic_) : 0), 
                  OB_MEMPOOL_MAGIC);
        result_errno = EINVAL;
      }
      else if (block_info->block_size_ != mem_block_size_)
      {
        mod_id = item_info->mod_id_;
        buf = reinterpret_cast<char*>(block_info);
        used_mem_block_num_ --;
        mem_size_handled_ -= block_info->block_size_;
        block_info->block_link_.remove();
        direct_allocated_block_num_ --;
        direct_allocated_mem_size_ -= block_info->block_size_; 
        size_free = block_info->block_size_;
        need_free = true;
      }
      else
      {
        mod_id = item_info->mod_id_;
        block_info->ref_num_ --;
        size_free = block_info->block_size_;
        if (block_info->ref_num_ == 0)
        {
          block_info->block_link_.remove();
          free_mem_block_list_.insert_next(block_info->block_link_);
          free_mem_block_num_ ++;
          used_mem_block_num_ --;
        }
      }
    }
    if (0 == result_errno)
    {
      mod_free(size_free,mod_id);
    }
  }

  if (need_free)
  {
    if (numa_enabled_)
    {
      numa_free(block_info, size_free);
    }
    else
    {
      delete [] block_info;
    }
    block_info = NULL;
  }
  errno = result_errno;
}

int64_t oceanbase::common::ObFixedMemPool::get_block_size() const
{
  return mem_block_size_;
}

int64_t oceanbase::common::ObFixedMemPool::shrink(const int64_t remain_memory_size)
{
  int result_errno = 0;
  int64_t memory_freed = 0;
  tbsys::CThreadGuard guard(&pool_mutex_);
  if (mem_block_size_ <= 0)
  {
    TBSYS_LOG(WARN, "memory pool not initialized");
    result_errno = EINVAL;
  }
  if (0 == result_errno)
  {
    ObDLink *block_it = free_mem_block_list_.next();
    while (block_it != &free_mem_block_list_ 
           && mem_size_handled_ > remain_memory_size)
    {
      memory_freed += recycle_memory_block(block_it);
    }
  }
  return memory_freed;
}

oceanbase::common::ObVarMemPool::ObVarMemPool(const int64_t block_size)
{
  oceanbase::common::ObDLink *link = NULL;
  link = new(&used_mem_block_list_)oceanbase::common::ObDLink;
  link = new(&free_mem_block_list_)oceanbase::common::ObDLink;
  used_mem_block_num_ = 0;
  free_mem_block_num_ = 0;
  cur_block_used_nbyte_ = 0;
  cur_block_info_ = NULL;
  direct_allocated_block_num_ = 0;
  direct_allocated_mem_size_ = 0;
  if (block_size < OB_MINIMUM_MALLOC_MEM_SIZE - 
      static_cast<int64_t>(sizeof(MemBlockInfo)) - 
      static_cast<int64_t>(sizeof(MemPoolItemInfo)))
  {
    mem_block_size_ = OB_MINIMUM_MALLOC_MEM_SIZE;
  }
  else
  {
    mem_block_size_ = block_size + sizeof(MemBlockInfo) + sizeof(MemPoolItemInfo);
  }
}


oceanbase::common::ObVarMemPool::~ObVarMemPool()
{
  clear(true);
}

int64_t oceanbase::common::ObVarMemPool::get_block_num() const
{
  tbsys::CThreadGuard guard(&pool_mutex_);
  return used_mem_block_num_ + free_mem_block_num_;
}

int64_t oceanbase::common::ObVarMemPool::get_used_block_num()const
{
  tbsys::CThreadGuard guard(&pool_mutex_);
  return used_mem_block_num_;
}

int64_t oceanbase::common::ObVarMemPool::recycle_memory_block(ObDLink * &block_it, 
                                                              bool check_unfreed_mem)
{
  int64_t memory_freed = 0;
  char *buf_allocated = NULL;
  MemBlockInfo *info = CONTAINING_RECORD(block_it, MemBlockInfo, block_link_);
  mem_size_handled_ -= info->block_size_;
  memory_freed += info->block_size_;
  block_it = info->block_link_.next();
  info->block_link_.remove();
  buf_allocated = reinterpret_cast<char*>(info);
  if (info->ref_num_ != 0)
  {
    used_mem_block_num_ --;
    if (check_unfreed_mem)
    {
      TBSYS_LOG(ERROR, "memory not released [begin_addr:%p,"
                "end_addr:%p]",buf_allocated, buf_allocated + info->block_size_);
    }
  }
  else
  {
    free_mem_block_num_ --;
  }
  if (numa_enabled_)
  {
    numa_free(buf_allocated, info->block_size_);
  }
  else
  {
    delete [] buf_allocated;
  }
  buf_allocated = NULL;
  return memory_freed;
}

int64_t oceanbase::common::ObVarMemPool::get_block_size() const
{
  return mem_block_size_;
}

int64_t oceanbase::common::ObVarMemPool::shrink(const int64_t remain_memory_size)
{
  int64_t memory_freed = 0;
  tbsys::CThreadGuard guard(&pool_mutex_);
  ObDLink *block_it = free_mem_block_list_.next();
  while (block_it != &free_mem_block_list_ 
         && mem_size_handled_ > remain_memory_size)
  {
    memory_freed += recycle_memory_block(block_it);
  }
  return memory_freed;
}

void oceanbase::common::ObVarMemPool::clear()
{
  clear(false);
}
void oceanbase::common::ObVarMemPool::clear(bool check_unfreed_mem)
{
  tbsys::CThreadGuard guard(&pool_mutex_);
  ObDLink *block_it = free_mem_block_list_.next();
  while (block_it != &free_mem_block_list_)
  {
    recycle_memory_block(block_it, check_unfreed_mem);
  }
  block_it = used_mem_block_list_.next();
  while (block_it != & used_mem_block_list_)
  {
    recycle_memory_block(block_it, check_unfreed_mem);
  }
  if (cur_block_info_ != NULL)
  {
    block_it = &((reinterpret_cast<MemBlockInfo*>(cur_block_info_))->block_link_);
    if (block_it != NULL)
    {
      recycle_memory_block(block_it, check_unfreed_mem);
      cur_block_info_ = NULL;
      cur_block_used_nbyte_ = 0;
    }
  }
  direct_allocated_block_num_ = 0;
  direct_allocated_mem_size_ = 0;
}


void  oceanbase::common::ObVarMemPool::free_(const void *ptr)
{
  int result_errno = 0;
  bool freed = false;
  bool need_free = false;
  const MemPoolItemInfo *item_info = NULL;
  MemBlockInfo *block_info = NULL;
  const char *buf = reinterpret_cast<const char*>(ptr);
  if (ptr == NULL)
  {
    freed = true;
  }
  if (malloc_directly())
  {
    delete [] reinterpret_cast<const char*>(ptr);
    freed = true;
  }
  if (!freed)
  {
    tbsys::CThreadGuard guard(&pool_mutex_);
    buf -= sizeof(MemPoolItemInfo);
    item_info = reinterpret_cast<const MemPoolItemInfo*>(buf);
    block_info = item_info->mother_block_;
    if (!check_mem_pool_item_info(*item_info))
    {
      TBSYS_LOG(ERROR, "memory corrupt [addr:%p,"
                "MemPoolItemInfo::magic_:%u,"
                "MemPoolItemInfo::mother_block_:%p,"
                "MemBlockInfo::magic_:%u,"
                "correct magic number:%u]",ptr, item_info->magic_,
                block_info,
                ((block_info != NULL) ? (block_info->magic_) : 0), 
                OB_MEMPOOL_MAGIC);
      result_errno = EINVAL;
    }
    else if (block_info->block_size_ != mem_block_size_)
    {
      buf = reinterpret_cast<char*>(block_info);
      used_mem_block_num_ --;
      mem_size_handled_ -= block_info->block_size_;
      block_info->block_link_.remove();
      direct_allocated_block_num_ --;
      direct_allocated_mem_size_ -= block_info->block_size_;
      need_free = true;
    }
    else
    {
      block_info->ref_num_ --;
      if (block_info->ref_num_ == 0)
      {
        block_info->block_link_.remove();
        free_mem_block_list_.insert_next(block_info->block_link_);
        free_mem_block_num_ ++;
        used_mem_block_num_ --;
        if (block_info == cur_block_info_)
        {
          cur_block_info_ = NULL;
          cur_block_used_nbyte_ = 0;
        }
      }
    }
  }

  if (need_free)
  {
    if (numa_enabled_)
    {
      numa_free(block_info, block_info->block_size_);
    }
    else
    {
      delete [] block_info;
    }
    block_info = NULL;
  }
  errno = result_errno;
}


void * oceanbase::common::ObVarMemPool::malloc_(const int64_t nbyte,const int32_t mod_id, int64_t *got_size)
{
  char *result = NULL;
  UNUSED(mod_id);
  int result_errno = 0;
  UNUSED(got_size);
  MemBlockInfo *block_info = NULL;
  MemPoolItemInfo *item_info = NULL;
  if (nbyte <= 0)
  {
    TBSYS_LOG(WARN, "cann't allocate memory less than 0 byte [nbyte:%ld]", nbyte);
    result_errno = EINVAL;
  }
  int64_t block_size = nbyte+sizeof(MemBlockInfo) + sizeof(MemPoolItemInfo);
  if (malloc_directly())
  {
    result = new(std::nothrow) char[nbyte];
    if (NULL == result)
    {
      result_errno = errno;
    }
  }

  /// allocated from system
  if (NULL ==result && result_errno == 0 && block_size > mem_block_size_)
  {
    result =reinterpret_cast<char*>(allocate_new_block(block_info,item_info,block_size,0,numa_enabled_));
    if (result == NULL)
    {
      result_errno = errno;
    }
    else
    {
      tbsys::CThreadGuard guard(&pool_mutex_);
      used_mem_block_list_.insert_next(block_info->block_link_);
      /// statistic 
      mem_size_handled_ += block_info->block_size_;
      used_mem_block_num_ ++;
      direct_allocated_block_num_ ++;
      direct_allocated_mem_size_ += block_info->block_size_;
    }
  }

  {
    tbsys::CThreadGuard guard(&pool_mutex_);
    block_info = reinterpret_cast<MemBlockInfo*>(cur_block_info_);
    /// current block's left memory is enough
    if (result_errno == 0 && result == NULL && block_info != NULL)
    {
      if (block_info->block_size_ - cur_block_used_nbyte_ >= 
          static_cast<int64_t>(sizeof(MemPoolItemInfo)) + nbyte)
      {
        item_info = reinterpret_cast<MemPoolItemInfo*>(reinterpret_cast<char*>(block_info) 
                                                       + cur_block_used_nbyte_);
        init_mem_pool_item_info(*item_info,block_info);
        cur_block_used_nbyte_ += nbyte + sizeof(MemPoolItemInfo);
        result  = item_info->buf_;
        block_info->ref_num_ ++;
      }
      else
      {
        /// current block's left memory is not enough
        used_mem_block_list_.insert_next(block_info->block_link_);
        block_info = NULL;
        cur_block_info_ = NULL;
        cur_block_used_nbyte_ = 0;
        /// statistic
        used_mem_block_num_ ++;
        free_mem_block_num_ --;
      }
    }
  }

  if (result_errno == 0 && result == NULL && block_info == NULL)
  {
    /// get from free list
    tbsys::CThreadGuard guard(&pool_mutex_);
    if (!free_mem_block_list_.is_empty())
    {
      block_info = CONTAINING_RECORD(free_mem_block_list_.next(), 
                                     MemBlockInfo, block_link_);
      item_info = reinterpret_cast<MemPoolItemInfo*>(block_info->buf_);
      init_mem_pool_item_info(*item_info,block_info);
      block_info->block_link_.remove();
      init_mem_block_info(*block_info,block_info->block_size_,1);
      cur_block_info_ = reinterpret_cast<void*>(block_info);
      cur_block_used_nbyte_ = sizeof(MemBlockInfo) + sizeof(MemPoolItemInfo) + nbyte;
      result = item_info->buf_;
    }
  }

  /// allocate new buffer
  if (result_errno == 0 && result == NULL && block_info == NULL)
  {
    result = reinterpret_cast<char*>(allocate_new_block(block_info,item_info,
                                                        mem_block_size_,0,numa_enabled_));
    if (result == NULL)
    {
      result_errno = errno;
    }
    else
    {
      tbsys::CThreadGuard guard(&pool_mutex_);
      cur_block_info_ = reinterpret_cast<void*>(block_info);
      cur_block_used_nbyte_ = sizeof(MemBlockInfo) + sizeof(MemPoolItemInfo) + nbyte;
      /// statistic
      used_mem_block_num_ ++;
      //free_mem_block_num_ ++;
      mem_size_handled_ += block_info->block_size_;
      result = item_info->buf_;
    }
  }
  errno = result_errno;
  return reinterpret_cast<void*>(result);
}

