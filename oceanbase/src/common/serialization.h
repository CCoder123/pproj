/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * serialization.h for ...
 *
 * Authors:
 *   maoqi <maoqi@taobao.com>
 *
 */
#ifndef OCEANBASE_COMMON_SERIALIZATION_H_
#define OCEANBASE_COMMON_SERIALIZATION_H_
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "ob_define.h"

namespace oceanbase
{
  namespace common 
  {
    namespace serialization 
    {

      const uint64_t OB_MAX_V1B = (__UINT64_C(1) << 7) - 1;
      const uint64_t OB_MAX_V2B = (__UINT64_C(1) << 14) - 1;
      const uint64_t OB_MAX_V3B = (__UINT64_C(1) << 21) - 1;
      const uint64_t OB_MAX_V4B = (__UINT64_C(1) << 28) - 1;
      const uint64_t OB_MAX_V5B = (__UINT64_C(1) << 35) - 1;
      const uint64_t OB_MAX_V6B = (__UINT64_C(1) << 42) - 1;
      const uint64_t OB_MAX_V7B = (__UINT64_C(1) << 49) - 1;
      const uint64_t OB_MAX_V8B = (__UINT64_C(1) << 56) - 1;
      const uint64_t OB_MAX_V9B = (__UINT64_C(1) << 63) - 1;


      const uint64_t OB_MAX_1B = (__UINT64_C(1) << 8) - 1;
      const uint64_t OB_MAX_2B = (__UINT64_C(1) << 16) - 1;
      const uint64_t OB_MAX_3B = (__UINT64_C(1) << 24) - 1;
      const uint64_t OB_MAX_4B = (__UINT64_C(1) << 32) - 1;
      const uint64_t OB_MAX_5B = (__UINT64_C(1) << 40) - 1;
      const uint64_t OB_MAX_6B = (__UINT64_C(1) << 48) - 1;
      const uint64_t OB_MAX_7B = (__UINT64_C(1) << 56) - 1;
      const uint64_t OB_MAX_8B = UINT64_MAX;



      const uint64_t OB_MAX_INT_1B = (__UINT64_C(23));
      const uint64_t OB_MAX_INT_2B = (__UINT64_C(1) << 8) - 1;
      const uint64_t OB_MAX_INT_3B = (__UINT64_C(1) << 16) - 1;
      const uint64_t OB_MAX_INT_4B = (__UINT64_C(1) << 24) - 1;
      const uint64_t OB_MAX_INT_5B = (__UINT64_C(1) << 32) - 1;
      const uint64_t OB_MAX_INT_7B = (__UINT64_C(1) << 48) - 1;
      const uint64_t OB_MAX_INT_9B = UINT64_MAX;
      
      
      const int64_t OB_MAX_1B_STR_LEN = (__INT64_C(55));
      const int64_t OB_MAX_2B_STR_LEN = (__INT64_C(1) << 8) - 1;
      const int64_t OB_MAX_3B_STR_LEN = (__INT64_C(1) << 16) - 1;
      const int64_t OB_MAX_4B_STR_LEN = (__INT64_C(1) << 24) - 1;
      const int64_t OB_MAX_5B_STR_LEN = (__INT64_C(1) << 32) - 1;

      const int8_t OB_INT_TYPE_BIT_POS = 7;
      const int8_t OB_INT_OPERATION_BIT_POS = 6;
      const int8_t OB_INT_SIGN_BIT_POS = 5;
      const int8_t OB_DATETIME_OPERATION_BIT = 3;
      const int8_t OB_DATETIME_SIGN_BIT = 2;
      const int8_t OB_FLOAT_OPERATION_BIT_POS = 0;

      const int8_t OB_INT_VALUE_MASK = 0x1f;
      const int8_t OB_VARCHAR_LEN_MASK = 0x3f;
      const int8_t OB_DATETIME_LEN_MASK =  0x3;


      const int8_t OB_VARCHAR_TYPE = (0x1 << 7);
      const int8_t OB_SEQ_TYPE = 0xc0;
      const int8_t OB_DATETIME_TYPE = 0xd0;
      const int8_t OB_PRECISE_DATETIME_TYPE = 0xe0;
      const int8_t OB_MODIFYTIME_TYPE = 0xf0;
      const int8_t OB_CREATETIME_TYPE = 0xf4;
      const int8_t OB_FLOAT_TYPE = 0xf8;
      const int8_t OB_DOUBLE_TYPE = 0xfa;
      const int8_t OB_NULL_TYPE =  0xfc;
      const int8_t OB_EXTEND_TYPE = 0xfe;

      inline void set_bit(int8_t& v,int8_t pos)
      {
        v |= (1 << pos);
      }

      inline void clear_bit(int8_t& v,int8_t pos)
      {
        v &= ~(1 << pos);
      }

      inline bool test_bit(int8_t v,int8_t pos)
      {
        return (v & (1 << pos));
      }

      /** 
       * @brief Encode a byte into given buffer.
       * 
       * @param buf address of destination buffer
       * @param val the byte
       *
       */

      inline int64_t encoded_length_i8(int8_t val)
      {
        return static_cast<int64_t>(sizeof(val));
      }

      inline int encode_i8(char *buf,const int64_t buf_len,int64_t& pos,int8_t val)
      {
        int ret =((NULL != buf) && ((buf_len - pos) >= static_cast<int64_t>(sizeof(val)))) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *(buf + pos++) = val;
        }
        return ret;
      }

      inline int decode_i8(const char *buf,const int64_t data_len,int64_t& pos,int8_t *val)
      {
        int ret = (NULL != buf && data_len - pos >= 1) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *val = *(buf + pos++);
        }
        return ret;
      }

      /** 
       * @brief Enocde a 16-bit integer in big-endian order
       * 
       * @param buf address of destination buffer
       * @param val value to encode
       */
      inline int64_t encoded_length_i16(int16_t val)
      {
        return static_cast<int64_t>(sizeof(val));
      }

      inline int encode_i16(char * buf,const int64_t buf_len,int64_t& pos,int16_t val)
      {
        int ret =((NULL != buf) && ((buf_len - pos) >= static_cast<int64_t>(sizeof(val)))) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *(buf + pos++) = ((((val) >> 8)) & 0xff);
          *(buf + pos++)= ((val) & 0xff);
        }
        return ret;
      }

      inline int decode_i16(const char * buf,const int64_t data_len,int64_t& pos,int16_t* val)
      {
        int ret = (NULL != buf && data_len - pos  >= 2) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *val = (static_cast<int16_t>((*(buf + pos++))) & 0xff) << 8;
          *val |= (*(buf + pos++) & 0xff);
        }
        return ret;
      }

      inline int64_t encoded_length_i32(int32_t val)
      {
        return static_cast<int64_t>(sizeof(val));
      }

      inline int encode_i32(char *buf,const int64_t buf_len,int64_t& pos,int32_t val)
      {
        int ret =((NULL != buf) && ((buf_len - pos) >= static_cast<int64_t>(sizeof(val)))) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *(buf + pos++) = ((val) >> 24) & 0xff;
          *(buf + pos++) = ((val) >> 16) & 0xff;
          *(buf + pos++) = ((val) >> 8) & 0xff;
          *(buf + pos++) = (val) & 0xff; 
        }
        return ret;
      }

      inline int decode_i32(const char * buf,const int64_t data_len,int64_t& pos,int32_t* val)
      {

        int ret = (NULL != buf && data_len - pos  >= 4) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *val = ((static_cast<int32_t>(*(buf + pos++))) & 0xff) << 24;
          *val |= ((static_cast<int32_t>(*(buf + pos++))) & 0xff) << 16;
          *val |= ((static_cast<int32_t>(*(buf + pos++))) & 0xff) << 8;
          *val |= ((static_cast<int32_t>(*(buf + pos++))) & 0xff); 
        }
        return ret;
      }

      inline int64_t encoded_length_i64(int64_t val)
      {
        return static_cast<int64_t>(sizeof(val));
      }

      inline int encode_i64(char *buf,const int64_t buf_len,int64_t& pos,int64_t val)
      {
        int ret =((NULL != buf) && ((buf_len - pos) >= static_cast<int64_t>(sizeof(val)))) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *(buf + pos++) = ((val) >> 56) & 0xff;
          *(buf + pos++) = ((val) >> 48) & 0xff;
          *(buf + pos++) = ((val) >> 40) & 0xff;
          *(buf + pos++) = ((val) >> 32) & 0xff;
          *(buf + pos++) = ((val) >> 24) & 0xff;
          *(buf + pos++) = ((val) >> 16) & 0xff;
          *(buf + pos++) = ((val) >> 8) & 0xff;
          *(buf + pos++) = (val) & 0xff;
        }
        return ret;
      }

      inline int decode_i64(const char * buf,const int64_t data_len,int64_t& pos,int64_t* val)
      {
        int ret = (NULL != buf && data_len - pos  >= 8) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          *val =  ((static_cast<int64_t>((*(buf + pos++))) & 0xff)) << 56;
          *val |= ((static_cast<int64_t>((*(buf + pos++))) & 0xff)) << 48;
          *val |= ((static_cast<int64_t>((*(buf + pos++))) & 0xff)) << 40;
          *val |= ((static_cast<int64_t>((*(buf + pos++))) & 0xff)) << 32;
          *val |= ((static_cast<int64_t>((*(buf + pos++))) & 0xff)) << 24;
          *val |= ((static_cast<int64_t>((*(buf + pos++))) & 0xff)) << 16;
          *val |= ((static_cast<int64_t>((*(buf + pos++))) & 0xff)) << 8;
          *val |= ((static_cast<int64_t>((*(buf + pos++))) & 0xff));
        }
        return ret;
      }

      inline int64_t encoded_length_bool(bool val)
      {
        UNUSED(val);
        return static_cast<int64_t>(1);
      }

      inline int encode_bool(char *buf,const int64_t buf_len,int64_t& pos,bool val)
      {
        int ret =((NULL != buf) && ((buf_len - pos) >= static_cast<int64_t>(sizeof(val)))) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
          *(buf + pos++) = (val) ? 1 : 0;
        return ret;
      }

      inline int decode_bool(const char *buf,const int64_t buf_len,int64_t& pos,bool *val)
      {
        int ret = OB_ERROR;
        int8_t v = 0;
        if ( (ret = decode_i8(buf,buf_len,pos,&v)) == 0 )
        {
          *val = (v != 0);
        }
        return ret;
      }

      inline int64_t encoded_length_vi64(int64_t val)
      {
        uint64_t __v = static_cast<uint64_t>(val);
        int64_t need_bytes= 0;
        if (__v <= OB_MAX_V1B)
          need_bytes = 1;
        else if (__v <= OB_MAX_V2B)
          need_bytes = 2;
        else if (__v <= OB_MAX_V3B)
          need_bytes = 3;
        else if (__v <= OB_MAX_V4B)
          need_bytes = 4;
        else if (__v <= OB_MAX_V5B)
          need_bytes = 5;
        else if (__v <= OB_MAX_V6B)
          need_bytes = 6;
        else if (__v <= OB_MAX_V7B)
          need_bytes = 7;
        else if (__v <= OB_MAX_V8B)
          need_bytes = 8;
        else if (__v <= OB_MAX_V9B)
          need_bytes = 9;
        else 
          need_bytes = 10;
        return need_bytes;
      }

      /** 
       * @brief Encode a integer (up to 64bit) in variable length encoding
       * 
       * @param buf pointer to the destination buffer
       * @param end the end pointer to the destination buffer
       * @param val value to encode
       * 
       * @return true - success, false - failed
       */
      inline int encode_vi64(char *buf,const int64_t buf_len,int64_t& pos,int64_t val)
      {
        uint64_t __v = static_cast<uint64_t>(val); 
        int ret =((NULL != buf) && ((buf_len - pos) >= encoded_length_vi64(__v))) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          while (__v > OB_MAX_V1B)
          {
            *(buf + pos++) = static_cast<int8_t>((__v) | 0x80);
            __v >>= 7;
          }
          if (__v <= OB_MAX_V1B)
          {
            *(buf + pos++)= static_cast<int8_t>((__v) & 0x7f);
          }
        }
        return ret;
      }

      inline int decode_vi64(const char *buf,const int64_t data_len,int64_t& pos,int64_t* val)
      {
        uint64_t __v = 0;
        uint32_t shift = 0;
        int64_t tmp_pos = pos;
        int ret = OB_SUCCESS;
        while( (*(buf + tmp_pos)) & 0x80)
        {
          if (data_len - tmp_pos < 1)
          {
            ret = OB_ERROR;
            break;
          }
          __v |=  (static_cast<uint64_t>(*(buf + tmp_pos++)) & 0x7f) << shift;
          shift += 7;
        }
        if (OB_SUCCESS == ret)
        {
          if (data_len - tmp_pos < 1)
          {
            ret = OB_ERROR;
          }
          else 
          {
            __v |= ((static_cast<uint64_t>(*(buf + tmp_pos++)) & 0x7f) << shift);
            *val = static_cast<int64_t>(__v);
            pos = tmp_pos;
          }
        }
        return ret;
      }

      inline int64_t encoded_length_vi32(int32_t val)
      {
        uint32_t __v = static_cast<uint64_t>(val);
        int64_t need_bytes= 0;
        if (__v <= OB_MAX_V1B)
          need_bytes = 1;
        else if (__v <= OB_MAX_V2B)
          need_bytes = 2;
        else if (__v <= OB_MAX_V3B)
          need_bytes = 3;
        else if (__v <= OB_MAX_V4B)
          need_bytes = 4;
        else
          need_bytes = 5;
        return need_bytes;

      }
     
      /** 
       * @brief Encode a integer (up to 32bit) in variable length encoding
       * 
       * @param buf pointer to the destination buffer
       * @param end the end pointer to the destination buffer
       * @param val value to encode
       * 
       * @return true - success, false - failed
       */

      inline int encode_vi32(char *buf,const int64_t buf_len,int64_t& pos,int32_t val)
      {
        uint32_t __v = static_cast<uint32_t>(val);
        int ret = ((NULL != buf) && ((buf_len - pos) >= encoded_length_vi32(val))) ? OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          while (__v > OB_MAX_V1B)
          {
            *(buf + pos++) = static_cast<int8_t>((__v) | 0x80);
            __v >>= 7;
          }
          if (__v <= OB_MAX_V1B)
          {
            *(buf + pos++) = static_cast<int8_t>((__v) & 0x7f);
          }
        }
        return ret;

      }

      inline int decode_vi32(const char *buf,const int64_t data_len,int64_t& pos,int32_t *val)
      {
        uint32_t __v = 0;
        uint32_t shift = 0;
        int ret = OB_SUCCESS;
        int64_t tmp_pos = pos;
        while( (*(buf + tmp_pos)) & 0x80)
        {
          if (data_len - tmp_pos < 1)
          {
            ret = OB_ERROR;
            break;
          }
          __v |=  (static_cast<uint32_t>(*(buf + tmp_pos++)) & 0x7f) << shift;
          shift += 7;
        }
        if (OB_SUCCESS == ret)
        {
          if (data_len - tmp_pos < 1)
            ret = OB_ERROR;
          else 
          {
            __v |=  (static_cast<uint32_t>(*(buf + tmp_pos++)) & 0x7f) << shift;
            *val = static_cast<int32_t>(__v);
            pos = tmp_pos;
          }
        }
        return ret;
      }

      inline int64_t encoded_length_float(float val)
      {
        int32_t tmp = 0;
        memcpy(&tmp,&val,sizeof(tmp));
        return encoded_length_vi32(tmp);
      }
      
      inline int encode_float(char *buf,const int64_t buf_len,int64_t& pos,float val)
      {
        int32_t tmp = 0;
        memcpy(&tmp,&val,sizeof(tmp));
        return encode_vi32(buf,buf_len,pos,tmp);
      }

      inline int decode_float(const char *buf,const int64_t data_len,int64_t& pos,float *val)
      {
        int32_t tmp = 0;
        int ret = OB_SUCCESS;
        if ((ret = decode_vi32(buf,data_len,pos,&tmp)) == 0)
        {
          memcpy(val,&tmp,sizeof(*val));
        }
        return ret;
      }
     
      inline int64_t encoded_length_double(double val)
      {
        int64_t tmp = 0;
        memcpy(&tmp,&val,sizeof(tmp));
        return encoded_length_vi64(tmp);
      }

      inline int encode_double(char *buf,const int64_t buf_len,int64_t& pos,double val)
      {
        int64_t tmp = 0;
        memcpy(&tmp,&val,sizeof(tmp));
        return encode_vi64(buf,buf_len,pos,tmp);
      }

      inline int decode_double(const char *buf,const int64_t data_len,int64_t& pos,double *val)
      {
        int64_t tmp = 0;
        int ret = OB_SUCCESS;
        if ((ret = decode_vi64(buf,data_len,pos,&tmp)) == 0)
        {
          memcpy(val,&tmp,sizeof(*val));
        }
        return ret;
      }

      /** 
       * @brief Computes the encoded length of vstr(int64,data,null)
       * 
       * @param len string length
       * 
       * @return the encoded length of str
       */
      inline int64_t encoded_length_vstr(int64_t len)
      {
        return encoded_length_vi64(len) + len + 1;
      }

      inline int64_t encoded_length_vstr(const char *str)
      {
        return encoded_length_vstr( str ? static_cast<int64_t>(strlen(str) + 1) : 0);
      }

      /** 
       * @brief get the decoded length of data len of vstr
       * won't change the pos 
       * @return the length of data
       */
      inline int64_t decoded_length_vstr(const char *buf,const int64_t data_len,int64_t pos)
      {  
        int64_t len = -1;
        int64_t tmp_pos = pos;
        if ( NULL == buf || data_len < 0 || pos < 0)
        {
          len = -1;
        }
        else if (decode_vi64(buf,data_len,tmp_pos,&len) != 0)
        {
          len = -1;
        }
        return len;
      }

      /** 
       * @brief Encode a buf as vstr(int64,data,null)
       * 
       * @param buf pointer to the destination buffer
       * @param vbuf pointer to the start of the input buffer
       * @param len length of the input buffer
       */
      inline int encode_vstr(char *buf,const int64_t buf_len,int64_t& pos,const void *vbuf, int64_t len)
      {
        int ret = ((NULL != buf) && (len >= 0) 
                    && ((buf_len - pos) >= static_cast<int32_t>(encoded_length_vstr(len)))) 
                  ?  OB_SUCCESS : OB_ERROR;
        if (OB_SUCCESS == ret)
        {
          /**
           * even through it's a null string, we can serialize it with 
           * lenght 0, and following a '\0'
           */
          ret = encode_vi64(buf,buf_len,pos,len);
          if (OB_SUCCESS == ret && len > 0 && NULL != vbuf)
          {
            memcpy(buf + pos,vbuf,len);
            pos += len;
          }
          *(buf + pos++) = 0;
        }
        return ret;
      }

      inline int encode_vstr(char *buf, const int64_t buf_len,int64_t& pos,const char *s)
      {
        return encode_vstr(buf,buf_len,pos,s,s ? strlen(s) + 1 : 0);
      }

      inline const char *decode_vstr(const char *buf,const int64_t data_len,int64_t& pos,int64_t *lenp)
      {
        const char *str = 0;
        int64_t tmp_len = 0;
        int64_t tmp_pos = pos;
        
        if ((NULL == buf) || (data_len < 0) || (pos < 0) || (NULL == lenp))
        {
          //just _return_;
        }
        else if ( decode_vi64(buf,data_len,tmp_pos,&tmp_len) != OB_SUCCESS)
        {
          *lenp = -1;
        }
        else if (tmp_len >= 0)
        {
          if ( data_len - tmp_pos >= tmp_len)
          {
            str = buf + tmp_pos;
            *lenp = tmp_len++;
            tmp_pos += tmp_len;
            pos = tmp_pos;
          }
          else
            *lenp = -1;
        }
        return str;
      }

      inline const char *decode_vstr(const char *buf,const int64_t data_len,int64_t& pos,
                                     char *dest,int64_t buf_len,int64_t *lenp)
      {
        const char *str = 0;
        int64_t tmp_len = 0;
        int64_t tmp_pos = pos;
        if (( NULL == buf)|| (data_len < 0) || (pos < 0) || (0 == dest) || buf_len < 0 || (NULL == lenp))
        {
          //just _return_;
        }
        else if ( decode_vi64(buf,data_len,tmp_pos,&tmp_len) != 0 || tmp_len > buf_len)
        {
          *lenp = -1;
        }
        else if (tmp_len >= 0)
        {
          if ( data_len - tmp_pos >= tmp_len)
          {
            str = buf + tmp_pos;
            *lenp = tmp_len++;
            memcpy(dest,str,*lenp);
            tmp_pos += tmp_len;
            pos = tmp_pos;
          }
          else
            *lenp = -1;
        }
        return str;
      }

      /** 
       * @brief encode ObNull type of ObObject into given buffer
       * 
       * @param buf pointer to the destination buffer
       * @param buf_len the length of the destination buffer
       * @param pos the current position of the destination buffer
       * 
       * @return on success,OB_SUCCESS is returned, on error,OB_ERROR is returned.
       */

      inline int64_t encoded_length_null()
      {
        return static_cast<int64_t>(1);
      }

      inline int encode_null(char *buf,int64_t buf_len,int64_t& pos)
      {
        return encode_i8(buf,buf_len,pos,OB_NULL_TYPE);
      }

      inline uint64_t safe_int64_abs(int64_t val)
      {
        int64_t __v = val;
        int64_t tmp = __v >> 63;
        __v ^= tmp;
        __v -= tmp;
        return static_cast<uint64_t>(__v);
      }
      

      /** 
       * @brief encode a uint64_t into given buffer with given length
       * 
       * @param buf       pointer to the destination buffer
       * @param buf_len   the length of buf
       * @param pos       the current position of the buffer
       * @param val       the value to encode
       * @param val_len   the encoded length of val in bytes
       * 
       * @return  on success, OB_SUCCESS is returned
       */
      inline int __encode_uint_with_given_length(char *buf, const int64_t buf_len,
                                                 int64_t& pos,
                                                 const uint64_t val, int64_t val_len)
      {
        int ret = OB_SUCCESS;
        if ((NULL == buf) || (val_len <= 0) || ((buf_len - pos) < val_len))
        {
          ret = OB_ERROR;
        }
        else
        {
          uint64_t __v = val;
          int64_t tmp_pos = pos;
          for(int64_t n=0;n < val_len; ++n)
          {
            int8_t t = ((__v >> (n << 3)) & 0xff);
            ret = encode_i8(buf,buf_len,tmp_pos,t);
            if (OB_SUCCESS != ret)
            {
              break;
            }
          }
          if (OB_SUCCESS == ret)
          {
            pos = tmp_pos;
          }
        }
        return ret;
      }

      inline int __decode_uint_with_given_length(const char *buf,const int64_t data_len,
                                                 int64_t& pos,
                                                 uint64_t& val,int64_t val_len)
      {
        int ret = OB_SUCCESS;
        if ((NULL == buf) || (data_len - pos < val_len))
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t tmp = 0;
          uint64_t __v = 0;
          int64_t tmp_pos = pos;
          for(int64_t n=0;n < val_len; ++n)
          {
            if ( OB_SUCCESS != (ret = decode_i8(buf,data_len,tmp_pos,&tmp)))
            {
              ret = OB_ERROR;
              break;
            }
            else
            {
              __v |= (static_cast<uint64_t>((static_cast<uint8_t>(tmp))) << (n << 3));
            }
          }

          if (OB_SUCCESS == ret)
          {
            val = __v;
            pos = tmp_pos;
          }
        }
        return ret;
      }

      /** 
       * @brief Computes the encoded length of int(type,val)
       * 
       * @param val the int value to be encoded
       * 
       * @return the encoded length of int
       */
      inline int64_t encoded_length_int(int64_t val)
      {
        uint64_t __v = safe_int64_abs(val);
        int64_t len = 0;
        if (__v <= OB_MAX_INT_1B)
          len = 1;
        else if(__v <= OB_MAX_INT_2B)
          len = 2;
        else if(__v <= OB_MAX_INT_3B)
          len = 3;
        else if(__v <= OB_MAX_INT_4B)
          len = 4;
        else if(__v <= OB_MAX_INT_5B)
          len = 5;
        else if(__v <= OB_MAX_INT_7B)
          len = 7;
        else /*if(__v < OB_MAX_INT_9B)*/
          len = 9;
        return len;
      }


      /** 
       * @brief encoded the ObInt type of ObObject into given buffer
       * 
       * @param buf pointer to the destination buffer
       * @param buf_len the length of buf
       * @param pos the current position of the buffer
       * @param val the value of int
       * @param is_add operation type
       * 
       * @return on success,OB_SUCCESS is returned,on error,OB_ERROR is returned.
       */
      inline int encode_int(char *buf,const int64_t buf_len,int64_t& pos,int64_t val,bool is_add = false)
      {
        int ret = OB_ERROR;
        int64_t len = encoded_length_int(val);
        if ( (NULL == buf) || ((buf_len - pos) < len))
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t first_byte = 0;
          if (val < 0)
          {
            set_bit(first_byte,OB_INT_SIGN_BIT_POS);
          }

          if (is_add)
          {
            set_bit(first_byte,OB_INT_OPERATION_BIT_POS);
          }
          uint64_t __v = safe_int64_abs(val);

          if (__v <= OB_MAX_INT_1B)
          {
            first_byte |= __v;
          }
          else
          {
            //24: 1 byte
            //25: 2 bytes
            //26: 3 bytes
            //27: 4 bytes
            //28: 6 bytes
            //29: 8 bytes
            //30,31 reserved
            int64_t tmp_len = len;
            if (7 == tmp_len || 9 == tmp_len)
            {
              tmp_len -= 1;
            }
            first_byte |= ((tmp_len - 1) + OB_MAX_INT_1B);
          }

          if ( OB_SUCCESS != (ret = encode_i8(buf,buf_len,pos,first_byte)))
          {
            ret = OB_ERROR;
          }
          else
          {
            if ((len - 1) > 0)
            {
              ret = __encode_uint_with_given_length(buf,buf_len,pos,__v,len - 1);
            }
          }
        }
        return ret;
      }

      inline int decode_int(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,int64_t& val,bool& is_add)
      {
        int ret = OB_ERROR;
        if ((NULL == buf) || (data_len - pos < 0))
        {
          ret = OB_ERROR;
        }
        else
        {
          bool is_neg = test_bit(first_byte,OB_INT_SIGN_BIT_POS);
          is_add = test_bit(first_byte,OB_INT_OPERATION_BIT_POS);
          int8_t len_or_value = first_byte & OB_INT_VALUE_MASK;
          int64_t __v = 0;

          if (len_or_value <= static_cast<int8_t>(OB_MAX_INT_1B))
          {
            __v = is_neg ? static_cast<int64_t>(-len_or_value) : static_cast<int64_t>(len_or_value);
            ret = OB_SUCCESS;
            val = __v;
          }
          else
          {
            int64_t len = len_or_value - OB_MAX_INT_1B;
            if ( 5 == len || 7 == len)
            {
              len += 1;
            }
            uint64_t uv = 0;
            ret = __decode_uint_with_given_length(buf,data_len,pos,uv,len);
            if (OB_SUCCESS == ret)
            {
              __v = static_cast<int64_t>(uv);
              val = is_neg ? -__v : __v;
            }
          }
        }
        return ret;
      }

      inline int64_t encoded_length_str_len(int64_t len)
      {
        int64_t ret = -1; 
        if (len < 0)
        {
          ret = -1;
        }
        else
        {
          if (len <= OB_MAX_1B_STR_LEN)
            ret = 1;
          else if(len <= OB_MAX_2B_STR_LEN)
            ret = 2;
          else if(len <= OB_MAX_3B_STR_LEN)
            ret = 3;
          else if(len <= OB_MAX_4B_STR_LEN)
            ret = 4;
          else if (len <= OB_MAX_5B_STR_LEN)
            ret = 5;
          else
            ret = -1;
        }
        return ret;
      }
      
      inline int64_t encoded_length_str(int64_t len)
      {
        return encoded_length_str_len(len) + len;
      }

      inline int encode_str(char *buf,const int64_t buf_len,int64_t& pos,const void *vbuf,int64_t len)
      {
        int ret = OB_ERROR;
        int64_t len_size = encoded_length_str_len(len);
        if ((NULL == buf) || (len_size < 0) || (buf_len - pos < len_size + len) || pos < 0
            || (len < 0))
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t first_byte = OB_VARCHAR_TYPE;

          if ( 1 == len_size)
          {
            first_byte |= (len & 0xff);
          }
          else
          {
            first_byte |= (len_size  - 1 + OB_MAX_1B_STR_LEN);
          }

          if (OB_SUCCESS != (ret = encode_i8(buf,buf_len,pos,first_byte)))
          {
            ret = OB_ERROR;
          }
          else if (len_size > 1)
          {
            for(int n=0;n < len_size - 1;++n)
            {
              if (OB_SUCCESS != (ret = encode_i8(buf,buf_len,pos,len >> (n << 3))))
              {
                break;
              }
            }
          }

          if (OB_SUCCESS == ret && (NULL != vbuf) & (len > 0))
          {
            memcpy(buf + pos,vbuf,len);
            pos += len;
          }
        }
        return ret;
      }

      inline const char *decode_str(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,int32_t& lenp)
      {
        const char *str = NULL;
        int ret = OB_ERROR;
        if ((NULL == buf) || data_len < 0)
        {
          str = NULL;
        }
        else
        {
          int8_t len_or_value = first_byte & OB_VARCHAR_LEN_MASK;
          int64_t str_len = 0;
          if (len_or_value <= OB_MAX_1B_STR_LEN)
          { 
            //OK ,we have already got the length of str
            str_len = static_cast<int64_t>(len_or_value);
            ret = OB_SUCCESS;
          }
          else
          {
            int8_t tmp = 0;
            for(int n=0;n<len_or_value - OB_MAX_1B_STR_LEN;++n)
            {
              if (OB_SUCCESS != (ret = decode_i8(buf,data_len,pos,&tmp)))
              {
                str_len = -1;
                break;
              }
              else
              {
                str_len |= (static_cast<int64_t>((static_cast<uint8_t>(tmp))) << (n << 3));
              }
            }
          }

          if ((OB_SUCCESS == ret) && (str_len >= 0) && (data_len - pos >= str_len))
          {
            str = buf + pos;
            pos += str_len;
            lenp = str_len;
          }
          else
          {
            lenp = -1;
            str = NULL;
          }

        }
        return str;
      }

      inline int64_t encoded_length_float_type()
      {
        return static_cast<int64_t>(sizeof(float) + 1);
      }

      inline int encode_float_type(char *buf,const int64_t buf_len,int64_t& pos,const float val,const bool is_add)
      {
        int32_t tmp = 0;
        memcpy(&tmp,&val,sizeof(tmp));
        int ret = OB_ERROR;
        int8_t first_byte = OB_FLOAT_TYPE;

        if (is_add)
        {
          set_bit(first_byte,OB_FLOAT_OPERATION_BIT_POS);
        }

        if ( OB_SUCCESS != (ret = encode_i8(buf,buf_len,pos,first_byte)))
        {
          ret = OB_ERROR;
        }
        else
        {
          ret = encode_i32(buf,buf_len,pos,tmp);
        }

        return ret;
      }

      inline int decode_float_type(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,float& val,bool& is_add)
      {
        int ret = OB_ERROR;
        if (NULL == buf || data_len - pos < static_cast<int64_t>(sizeof(float)))
        {
          ret = OB_ERROR;
        }
        else 
        {
          int32_t tmp = 0;
          if ( OB_SUCCESS != (ret = decode_i32(buf,data_len,pos,&tmp)))
          {
            ret = OB_ERROR;
          }
          else
          {
            memcpy(&val,&tmp,sizeof(val));
            ret = OB_SUCCESS;
          }

          if (OB_SUCCESS == ret)
          {
            is_add = serialization::test_bit(first_byte,OB_FLOAT_OPERATION_BIT_POS);
          }
        }
        return ret;
      }

      inline int64_t encoded_length_double_type()
      {
        return static_cast<int64_t>(sizeof(double) + 1);
      }

      inline int encode_double_type(char *buf,const int64_t buf_len,int64_t& pos,double val,const bool is_add)
      {
        int64_t tmp = 0;
        memcpy(&tmp,&val,sizeof(tmp));

        int ret = OB_ERROR;

        int8_t first_byte = OB_DOUBLE_TYPE;

        if (is_add)
        {
          set_bit(first_byte,OB_FLOAT_OPERATION_BIT_POS);
        }

        if ( OB_SUCCESS != (ret = encode_i8(buf,buf_len,pos,first_byte)))
        {
          ret = OB_ERROR;
        }
        else
        {
          ret = encode_i64(buf,buf_len,pos,tmp);
        }

        return ret;
      }
      
      inline int decode_double_type(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,double& val,bool& is_add)
      {
        int ret = OB_ERROR;
        if (NULL == buf || data_len - pos < static_cast<int64_t> (sizeof(double)))
        {
          ret = OB_ERROR;
        }
        else 
        {
          int64_t tmp = 0;
          if ( OB_SUCCESS != (ret = decode_i64(buf,data_len,pos,&tmp)))
          {
            ret = OB_ERROR;
          }
          else
          {
            memcpy(&val,&tmp,sizeof(val));
            is_add = test_bit(first_byte,OB_FLOAT_OPERATION_BIT_POS);
          }
        }
        return ret;
      }

      inline int64_t encoded_length_datetime(int64_t value)
      {
        int64_t len = 0;
        uint64_t tmp_value = safe_int64_abs(value);
        if (tmp_value <= OB_MAX_4B)
          len = 5;
        else if (tmp_value <= OB_MAX_6B)
          len = 7;
        else 
          len = 9;
        return len;
      }

      inline int64_t encoded_length_precise_datetime(int64_t value)
      {
        return encoded_length_datetime(value);
      }

      inline int __encode_time_type(char *buf,const int64_t buf_len,int8_t first_byte,int64_t& pos,int64_t val)
      {
        int64_t len = encoded_length_datetime(val);
        int64_t tmp_pos = pos;
        int ret = OB_SUCCESS;
        if (NULL == buf || buf_len - pos < len)
        {
          ret = OB_ERROR;
        }
        else
        {
          if (7 == len)
          {
            first_byte |= 1;
          }
          else if(9 == len)
          {
            first_byte |= 2;
          }
          
          if ( OB_SUCCESS == ( ret = encode_i8(buf,buf_len,tmp_pos,first_byte)))
          {
            uint64_t __v = safe_int64_abs(val);
            ret = __encode_uint_with_given_length(buf,buf_len,tmp_pos,__v,len - 1);
          }
          if (OB_SUCCESS == ret)
          {
            pos = tmp_pos;
          }
        }
        return ret;
      }

      inline int __decode_time_type(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,int64_t& val)
      {
        int ret = OB_ERROR;
        if ((NULL == buf) || (data_len - pos < 0))
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t len_mark = first_byte & OB_DATETIME_LEN_MASK;
          int64_t len = 0;
          if (0 == len_mark)
          {
            len = 4;
          }
          else if (1 == len_mark)
          {
            len = 6;
          }
          else if(2 == len_mark)
          {
            len = 8;
          }

          if (0 == len)
          {
            ret = OB_ERROR;
          }
          else
          {
            uint64_t uv = 0;
            ret = __decode_uint_with_given_length(buf,data_len,pos,uv,len);
            if (OB_SUCCESS == ret)
            {
              val = static_cast<int64_t>(uv);
            }
          }
        }
        return ret;
      }

      inline int encode_datetime_type(char *buf,const int64_t buf_len,int64_t& pos,const ObDateTime& val,const bool is_add)
      {
        int ret = OB_ERROR;
        if (NULL == buf || buf_len - pos <= 0)
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t first_byte = OB_DATETIME_TYPE;

          if (val < 0)
          {
            set_bit(first_byte,OB_DATETIME_SIGN_BIT);
          }

          if (is_add)
          {
            set_bit(first_byte,OB_DATETIME_OPERATION_BIT);
          }

          ret = __encode_time_type(buf,buf_len,first_byte,pos,val);

        }
        return ret;
      }

      inline int decode_datetime_type(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,int64_t& val,bool& is_add)
      {
        int ret = OB_ERROR;
        if ((NULL == buf) || (data_len - pos < 0))
        {
          ret = OB_ERROR;
        }
        else
        {
          bool is_neg = test_bit(first_byte,OB_DATETIME_SIGN_BIT);
          is_add = test_bit(first_byte,OB_DATETIME_OPERATION_BIT);

          int64_t __v = 0;

          if ( OB_SUCCESS == (ret = __decode_time_type(buf,data_len,first_byte,pos,__v)))
          { 
            val = is_neg ? -__v : __v;
          }
        }
        return ret;
      }

      inline int encode_precise_datetime_type(char *buf,const int64_t buf_len,int64_t& pos,const ObPreciseDateTime& val,const bool is_add)
      {
        int ret = OB_ERROR;
        if (NULL == buf || buf_len - pos <= 0 )
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t first_byte = OB_PRECISE_DATETIME_TYPE;

          if (val < 0)
          {
            set_bit(first_byte,OB_DATETIME_SIGN_BIT);
          }

          if (is_add)
          {
            set_bit(first_byte,OB_DATETIME_OPERATION_BIT);
          }

          ret = __encode_time_type(buf,buf_len,first_byte,pos,val);

        }
        return ret;
      }
      
      inline int decode_precise_datetime_type(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,int64_t& val,bool& is_add)
      {
        int ret = OB_ERROR;
        if ((NULL == buf) || (data_len - pos < 0))
        {
          ret = OB_ERROR;
        }
        else
        {
          bool is_neg = test_bit(first_byte,OB_DATETIME_SIGN_BIT);
          is_add = test_bit(first_byte,OB_DATETIME_OPERATION_BIT);

          int64_t __v = 0;

          if ( OB_SUCCESS == (ret = __decode_time_type(buf,data_len,first_byte,pos,__v)))
          { 
            val = is_neg ? -__v : __v;
          }
        }
        return ret;
      }

      inline int64_t encoded_length_modifytime(int64_t value)
      {
        return encoded_length_datetime(value);
      }

      inline int encode_modifytime_type(char *buf,const int64_t buf_len,int64_t& pos,const ObModifyTime& val)
      {
        int ret = OB_ERROR;
        if (NULL == buf || buf_len - pos <= 0 || val < 0)
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t first_byte = OB_MODIFYTIME_TYPE;
          ret = __encode_time_type(buf,buf_len,first_byte,pos,val);
        }
        return ret;
      }

      inline int decode_modifytime_type(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,int64_t& val)
      {
        int ret = OB_ERROR;
        if ((NULL == buf) || (data_len - pos <= 0))
        {
          ret = OB_ERROR;
        }
        else
        {
          ret = __decode_time_type(buf,data_len,first_byte,pos,val);
        }
        return ret;
      }

      inline int64_t encoded_length_createtime(const int64_t value)
      {
        return encoded_length_datetime(value);
      }

      inline int encode_createtime_type(char *buf,const int64_t buf_len,int64_t& pos,const ObCreateTime& val)
      {
        int ret = OB_ERROR;
        if (NULL == buf || buf_len - pos <= 0 || val < 0)
        {
          ret = OB_ERROR;
        }
        else
        {
          int8_t first_byte = OB_CREATETIME_TYPE;
          ret = __encode_time_type(buf,buf_len,first_byte,pos,val);
        }
        return ret;
      }

      inline int decode_createtime_type(const char *buf,const int64_t data_len,int8_t first_byte,int64_t& pos,int64_t& val)
      {
        int ret = OB_ERROR;
        if ((NULL == buf) || (data_len - pos <= 0))
        {
          ret = OB_ERROR;
        }
        else
        {
          ret = __decode_time_type(buf,data_len,first_byte,pos,val);
        }
        return ret;
      }

      inline int64_t encoded_length_extend(const int64_t value)
      {
        return encoded_length_vi64(value) + 1;
      }

      inline int64_t encode_extend_type(char *buf,const int64_t buf_len,int64_t& pos,const int64_t val)
      {
        int ret = OB_SUCCESS;
        int64_t len = encoded_length_extend(val);
        if (NULL == buf || buf_len < 0 || (buf_len - pos < len))
        {
          ret = OB_ERROR;
        }
        else
        {
          int64_t tmp_pos = pos;
          if ( OB_SUCCESS == ( ret = encode_i8(buf,buf_len,tmp_pos,OB_EXTEND_TYPE)))
          {
            ret = encode_vi64(buf,buf_len,tmp_pos,val);
          }

          if (OB_SUCCESS == ret)
          {
            pos = tmp_pos;
          }
        }
        return ret;
      }

    } /* serialization */
  } /* common */
} /* oceanbase*/

#endif //OCEANBASE_COMMON_SERIALIZATION_H

