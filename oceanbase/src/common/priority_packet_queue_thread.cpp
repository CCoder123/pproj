/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_common_param.cpp for ...
 *
 * Authors:
 *    chuanhui <rizhao.ych@taobao.com>
 *
 */

#include "priority_packet_queue_thread.h"
#include "ob_probability_random.h"

using namespace tbnet;

namespace oceanbase {
namespace common {

// 构造
PriorityPacketQueueThread::PriorityPacketQueueThread() : tbsys::CDefaultRunnable()
{
  _stop = 0;
  _waitFinish = false;
  _handler = NULL;
  _args = NULL;
  for (int64_t i = 0; i < QUEUE_NUM; ++i)
  {
    _waiting[i] = false;
    _queues[i].init();
  }

  _percent[LOW_PRIV] = 10;
  _percent[NORMAL_PRIV] = 90;
  _sum = 100;
}

// 构造
PriorityPacketQueueThread::PriorityPacketQueueThread(int threadCount, IPacketQueueHandler *handler, void *args)
        : tbsys::CDefaultRunnable(threadCount)
{
  _stop = 0;
  _waitFinish = false;
  _handler = handler;
  _args = args;
  for (int64_t i = 0; i < QUEUE_NUM; ++i)
  {
    _waiting[i] = false;
  }

  _percent[LOW_PRIV] = 10;
  _percent[NORMAL_PRIV] = 90;
  _sum = 100;
}

// 析构
PriorityPacketQueueThread::~PriorityPacketQueueThread()
{
    stop();
}

// 线程参数设置
void PriorityPacketQueueThread::setThreadParameter(int threadCount, IPacketQueueHandler *handler, void *args)
{
    setThreadCount(threadCount);
    _handler = handler;
    _args = args;
}

// stop
void PriorityPacketQueueThread::stop(bool waitFinish)
{
    _waitFinish = waitFinish;
    _stop = true;
}

// push
// block==true, this thread can wait util _queue.size less than maxQueueLen
// otherwise, return false directly, client must be free this packet.
bool PriorityPacketQueueThread::push(ObPacket *packet, int maxQueueLen, bool block, int priority)
{
  // invalid param
  if (NULL == packet)
  {
    TBSYS_LOG(WARN, "packet is NULL");
    return false;
  }
  else if (priority != NORMAL_PRIV && priority != LOW_PRIV)
  {
    TBSYS_LOG(WARN, "invalid param, priority=%d", priority);
    return false;
  }

  // 是停止就不允许放了
  if (_stop) {
    return true;
  }

  // 是否要限制push长度
  if (maxQueueLen>0 && _queues[priority].size() >= maxQueueLen)
  {
    _pushcond[priority].lock();
    _waiting[priority] = true;
    while (_stop == false && _queues[priority].size() >= maxQueueLen && block)
    {
      _pushcond[priority].wait(1000);
    }
    _waiting[priority] = false;
    if (_queues[priority].size() >= maxQueueLen && !block)
    {
      _pushcond[priority].unlock();
      return false;
    }
    _pushcond[priority].unlock();

    if (_stop)
    {
      return true;
    }
  }

  // 加锁写入队列
  _cond[priority].lock();
  _queues[priority].push(packet);
  _cond[priority].unlock();
  _cond[priority].signal();
  return true;
}

ObPacket* PriorityPacketQueueThread::pop_packet_(const int64_t priority)
{
  ObPacket* packet = NULL;

  _cond[priority].lock();
  // 取出packet
  packet = _queues[priority].pop();
  _cond[priority].unlock();

  // push 在等吗?
  if (NULL != packet && _waiting[priority])
  {
    _pushcond[priority].lock();
    _pushcond[priority].signal();
    _pushcond[priority].unlock();
  }

  return packet;
}

// Runnable 接口
void PriorityPacketQueueThread::run(tbsys::CThread *, void *)
{
  Packet *packet = NULL;
  int64_t priority = -1;
  static const int64_t TASK_WAIT_TIME = 1; // wait 1ms if there is no task

  while (!_stop)
  {
    if (_queues[NORMAL_PRIV].size() == 0)
    {
      if (_queues[LOW_PRIV].size() == 0) // no task at all
      {
        // wait 1ms for normal task
        _cond[NORMAL_PRIV].lock();
        _cond[NORMAL_PRIV].wait(TASK_WAIT_TIME);
        _cond[NORMAL_PRIV].unlock();
        continue;
      }
      else // only low priv queue has task
      {
        priority = LOW_PRIV;
      }
    }
    else
    {
      if (_queues[LOW_PRIV].size() == 0) // only normal priv queue has task
      {
        priority = NORMAL_PRIV;
      }
      else
      {
        priority = ObStalessProbabilityRandom::random(_percent, QUEUE_NUM, _sum);
        if (priority >= QUEUE_NUM || priority < 0)
        {
          priority = NORMAL_PRIV;
        }
      }
    }

    if (_stop) break;

    packet = pop_packet_(priority);
    if (packet == NULL) continue;

    // handle packet
    if (_handler)
    {
      _handler->handlePacketQueue(packet, _args);
    }
  }

  // 把queue中所有的task做完
  if (_waitFinish)
  {
    for (int64_t priority = NORMAL_PRIV; priority <= LOW_PRIV; ++priority)
    {
      bool ret = true;
      _cond[priority].lock();
      while (_queues[priority].size() > 0) {
        packet = _queues[priority].pop();
        _cond[priority].unlock();
        ret = true;
        if (_handler) {
          ret = _handler->handlePacketQueue(packet, _args);
        }
        //if (ret) delete packet;

        _cond[priority].lock();
      }
      _cond[priority].unlock();
    }
  }
  else
  {
    // 把queue中的free掉
    for (int64_t priority = NORMAL_PRIV; priority <= LOW_PRIV; ++priority)
    {
      _cond[priority].lock();
      while (_queues[priority].size() > 0) {
        _queues[priority].pop();
      }
      _cond[priority].unlock();
    }
  }
}

}
}

