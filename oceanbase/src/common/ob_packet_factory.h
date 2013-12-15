/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_packet_factory.h for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */
#ifndef OCEANBASE_COMMON_PACKET_FACTORY_H_
#define OCEANBASE_COMMON_PACKET_FACTORY_H_

#include <tbnet.h>

#include "ob_define.h"
#include "ob_packet.h"
#include "thread_buffer.h"

namespace oceanbase
{
  namespace common
  {
    class ObPacketFactory : public tbnet::IPacketFactory
    {
      public:
        ObPacketFactory()
        {
          packet_buffer_ = new ThreadSpecificBuffer(THREAD_BUFFER_SIZE);
        }

        ~ObPacketFactory()
        {
          if (packet_buffer_ != NULL)
          {
            delete packet_buffer_;
            packet_buffer_ = NULL;
          }
        }

        tbnet::Packet* createPacket(int pcode)
        {
          UNUSED(pcode);
          ObPacket* packet = NULL;
          ThreadSpecificBuffer::Buffer* tb = packet_buffer_->get_buffer();
          if (tb == NULL)
          {
            TBSYS_LOG(ERROR, "get packet thread buffer failed, return NULL");
          }
          else
          {
            char* buf = tb->current();
            packet = new(buf) ObPacket();
            buf += sizeof(ObPacket);
            packet->set_packet_buffer(buf, OB_MAX_PACKET_LENGTH);
          }
          return packet;
        }

        void destroyPacket(tbnet::Packet* packet)
        {
          UNUSED(packet);
          // does nothing
        }

      private:
        static const int32_t THREAD_BUFFER_SIZE = sizeof(ObPacket) + OB_MAX_PACKET_LENGTH;

      private:
        ThreadSpecificBuffer *packet_buffer_;
    };
  } /* common */
} /* oceanbase */

#endif /* end of include guard: OCEANBASE_COMMON_PACKET_FACTORY_H_ */

