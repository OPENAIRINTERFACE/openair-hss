/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under 
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#if 0
#include "hss_config.h"
#include "db_proto.h"
#include "s6a_proto.h"
#endif

#define RAPIDJSON_NAMESPACE fdrapidjson

#include <stdio.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include <string>
#include <stdexcept>

#include "freeDiameter/freeDiameter-host.h"
#include "freeDiameter/libfdcore.h"
#include "freeDiameter/libfdproto.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "fdjson.h"

#define ISHEXDIGIT(x) \
( \
   (x >= '0' && x <= '9') || \
   (x >= 'a' && x <= 'f') || \
   (x >= 'A' && x <= 'F') \
)

#define HEX2BIN(x) ( \
   x >= '0' && x <= '9' ? x - '0' : \
   x >= 'a' && x <= 'f' ? x - 'a' + 10 : \
   x >= 'A' && x <= 'F' ? x - 'A' + 10 : 15 \
)

class runtimeInfo : public std::runtime_error
{
public:
   runtimeInfo(const char *m) : std::runtime_error(m) {}
   runtimeInfo(const std::string &m) : std::runtime_error(m) {}
};

class runtimeError : public std::runtime_error
{
public:
   runtimeError(const char *m) : std::runtime_error(m) {}
   runtimeError(const std::string &m) : std::runtime_error(m) {}
};

template<class T>
class Buffer
{
public:
   Buffer(size_t size) { msize = size; mbuf = new T[msize]; }
   ~Buffer() { if (mbuf) delete [] mbuf; }
   T *get() { return mbuf; }
private:
   Buffer();
   size_t msize;
   T *mbuf;
};

std::string string_format( const char *format, ... )
{
   va_list args;

   va_start( args, format );
   size_t size = vsnprintf( NULL, 0, format, args ) + 1; // Extra space for '\0'
   va_end( args );

   Buffer<char> buf( size );

   va_start( args, format );
   vsnprintf( buf.get(), size, format, args  );
   va_end( args );

   return std::string( buf.get(), size - 1 ); // We don't want the '\0' inside
}

class AVP
{
public:
   AVP( const char *avp_name ) { _init(avp_name); }
   AVP( const char *avp_name, int32_t v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, int64_t v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, uint32_t v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, uint64_t v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, float v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, double v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, const int8_t *v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, const int8_t *v, size_t len ) { _init(avp_name); set(v, len); }
   AVP( const char *avp_name, const uint8_t *v ) { _init(avp_name); set(v); }
   AVP( const char *avp_name, const uint8_t *v, size_t len ) { _init(avp_name); set(v, len); }

   ~AVP()
   {
      if (mBuf)
         delete mBuf;
   }

   AVP &set( int32_t v ) { mValue.i32 = v; return *this; }
   AVP &set( int64_t v ) { mValue.i64 = v; return *this; }
   AVP &set( uint32_t v ) { mValue.u32 = v; return *this; }
   AVP &set( uint64_t v ) { mValue.u64 = v; return *this; }
   AVP &set( float v ) { mValue.f32 = v; return *this; }
   AVP &set( double v ) { mValue.f64 = v; return *this; }
   AVP &set( const int8_t *v ) { return set(v, strlen((const char *)v)); }
   AVP &set( const int8_t *v, size_t len ) { mValue.os.data = (uint8_t*)v; mValue.os.len = len; return *this; }
   AVP &set( const uint8_t *v ) { return set(v, strlen((const char *)v)); }
   AVP &set( const uint8_t *v, size_t len ) { mValue.os.data = (uint8_t*)v; mValue.os.len = len; return *this; }

   void addTo( msg_or_avp *reference ) { _addTo(reference); }
   void addTo( AVP &reference ) { _addTo(reference.mAvp); }

   struct avp *getAvp() { return mAvp; }

   void allocBuffer(size_t len) { mBuf = new Buffer<uint8_t>(len); }
   Buffer<uint8_t> &getBuffer() { return *mBuf; }

   enum dict_avp_basetype getBaseType() { return mBaseData.avp_basetype; }

   bool isDerived() { return mIsDerived; }
   bool isAddress() { return mIsAddress; }

private:
   void _init( const char *avp_name )
   {
      int ret = 0;

      mName = avp_name;
      mBaseEntry = NULL;
      memset( &mBaseData, 0, sizeof(mBaseData) );
      mIsDerived = false;
      mIsAddress = false;
      mBuf = NULL;
      mAvp = NULL;
      memset( &mValue, 0, sizeof(mValue) );

      /* get the dictionary entry for the AVP */
      ret = fd_dict_search( fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME_ALL_VENDORS, mName, &mBaseEntry, ENOENT );
      if (ret != 0)
         throw runtimeInfo(
            string_format("%s:%d - INFO - Unable to find AVP dictionary entry  for [%s]",
            __FILE__, __LINE__, mName)
         );

      /* get the dictionary data for the AVP's dictionary entry */
      ret = fd_dict_getval( mBaseEntry, &mBaseData );
      if (ret != 0)
         throw runtimeInfo(
            string_format("%s:%d - INFO - Unable to retrieve the dictionary data for the [%s] dictionary entry",
            __FILE__, __LINE__, mName)
         );

      /* check if basetype of AVP is OCTET_STRING */
      if ( mBaseData.avp_basetype == AVP_TYPE_OCTETSTRING )
      {
         struct dictionary *dict = NULL;
         struct dict_object *derivedtype = NULL;

         /* get the dictionary associated with the AVP dictionary entry */
         ret = fd_dict_getdict( mBaseEntry, &dict );
         if (ret != 0)
            throw runtimeInfo(
               string_format("%s:%d - INFO - Unable to retrieve the dictionary for the [%s] dictionary entry",
               __FILE__, __LINE__, mName)
            );

         /* get the dictionary entry associated with the derived type */
         ret = fd_dict_search( dict, DICT_TYPE, TYPE_OF_AVP, mBaseEntry, &derivedtype, EINVAL );
         if (ret == 0) /* if found, then derived */
         {
            struct dict_type_data derived_type_data;

            ret = fd_dict_getval( derivedtype, &derived_type_data );
            
            if (ret == 0)
            {
               mIsDerived = true;
               mIsAddress = (!strcmp(derived_type_data.type_name,"Address")) ? true : false;
            }
         }
         else
         {
            /**** THIS IS NOT AN ERROR CONDITION ****/
            /*
            throw runtimeInfo(
               string_format("%s:%d - INFO - Unable to retrieve the derived type entry for dictionary entry [%s]",
               __FILE__, __LINE__, mName)
            );
            */
         }
      }
   }

   void _addTo( msg_or_avp *reference )
   {
      int ret;

      if ((ret = fd_msg_avp_new(mBaseEntry,0,&mAvp)) != 0)
         throw runtimeError(
            string_format("%s:%d - ERROR - Error [%d] creating [%s] AVP",
            __FILE__, __LINE__, ret, mName)
         );

      try
      {
         if ( mBaseData.avp_basetype != AVP_TYPE_GROUPED )
         {
            if ((ret = fd_msg_avp_setvalue(mAvp,&mValue)) != 0)
               throw runtimeError(
                  string_format("%s:%d - ERROR - Error [%d] setting AVP value for [%s]",
                  __FILE__, __LINE__, ret, mName)
               );
         }

         if ((ret = fd_msg_avp_add(reference,MSG_BRW_LAST_CHILD,mAvp)) != 0)
            throw runtimeError(
               string_format("%s:%d - ERROR - Error [%d] adding [%s] AVP",
               __FILE__, __LINE__, ret, mName)
            );
      }
      catch (...)
      {
         fd_msg_free( mAvp );
         mAvp = NULL;
         throw;
      }
   }

   const char * mName;
   struct dict_object *mBaseEntry;
   struct dict_avp_data mBaseData;
   bool mIsDerived;
   bool mIsAddress;
   Buffer<uint8_t> *mBuf;
   struct avp *mAvp;
   union avp_value mValue;
};

#define THROW_DATATYPE_MISMATCH() \
{ \
   throw runtimeInfo( string_format("%s:%d - INFO - Datatype mismatch for [%s] - expected datatype compatible with %s, JSON data type was %s", \
      __FILE__, __LINE__, name, \
      avp.getBaseType() == AVP_TYPE_GROUPED ? "AVP_TYPE_GROUPED" : \
      avp.getBaseType() == AVP_TYPE_OCTETSTRING ? "AVP_TYPE_OCTETSTRING" : \
      avp.getBaseType() == AVP_TYPE_INTEGER32 ? "AVP_TYPE_INTEGER32" : \
      avp.getBaseType() == AVP_TYPE_INTEGER64 ? "AVP_TYPE_INTEGER64" : \
      avp.getBaseType() == AVP_TYPE_UNSIGNED32 ? "AVP_TYPE_UNSIGNED32" : \
      avp.getBaseType() == AVP_TYPE_FLOAT32 ? "AVP_TYPE_FLOAT32" : \
      avp.getBaseType() == AVP_TYPE_FLOAT64 ? "AVP_TYPE_FLOAT64" : "UNKNOWN", \
      value.GetType() == RAPIDJSON_NAMESPACE::kNullType ? "kNullType" : \
      value.GetType() == RAPIDJSON_NAMESPACE::kFalseType ? "kFalseType" : \
      value.GetType() == RAPIDJSON_NAMESPACE::kTrueType ? "kTrueType" : \
      value.GetType() == RAPIDJSON_NAMESPACE::kObjectType ? "kObjectType" : \
      value.GetType() == RAPIDJSON_NAMESPACE::kArrayType ? "kArrayType" : \
      value.GetType() == RAPIDJSON_NAMESPACE::kStringType ? "kStringType" : \
      value.GetType() == RAPIDJSON_NAMESPACE::kNumberType ? "kNumber" : "Unknown") ); \
}

static void fdJsonAddAvps( AVP &avp, const RAPIDJSON_NAMESPACE::Value &element, void (*errfunc)(const char*) );
static void fdJsonAddAvps( msg_or_avp *reference, const RAPIDJSON_NAMESPACE::Value &element, void (*errfunc)(const char*) );

static void fdJsonAddAvp( msg_or_avp *reference, const char *name, const RAPIDJSON_NAMESPACE::Value &value, void (*errfunc)(const char*) )
{
   int ret;
   AVP avp( name );

   switch (value.GetType())
   {
      case RAPIDJSON_NAMESPACE::kFalseType:
      case RAPIDJSON_NAMESPACE::kTrueType:
      {
         throw runtimeInfo( string_format("%s:%d - INFO - Invalid format (true/false) for [%s] in JSON block, ignoring", __FILE__, __LINE__, name) );
         break;
      }
      case RAPIDJSON_NAMESPACE::kNullType:
      {
         throw runtimeInfo( string_format("%s:%d - INFO - Invalid NULL for [%s] in JSON block, ignoring", __FILE__, __LINE__, name) );
         break;
      }
      case RAPIDJSON_NAMESPACE::kNumberType:
      {
         switch ( avp.getBaseType() )
         {
            case AVP_TYPE_INTEGER32: {
               if (!value.IsInt()) THROW_DATATYPE_MISMATCH();
               avp.set( value.GetInt() );
               break;
            }
            case AVP_TYPE_INTEGER64: {
               if (!value.IsInt64()) THROW_DATATYPE_MISMATCH();
               avp.set( value.GetInt64() );
               break;
            }
            case AVP_TYPE_UNSIGNED32: {
               if (!value.IsUint()) THROW_DATATYPE_MISMATCH();
               avp.set( value.GetUint() );
               break;
            }
            case AVP_TYPE_UNSIGNED64: {
               if (!value.IsUint64()) THROW_DATATYPE_MISMATCH();
               avp.set( value.GetUint64() );
               break;
            }
            case AVP_TYPE_FLOAT32: {
               if (!value.IsFloat()) THROW_DATATYPE_MISMATCH();
               avp.set( value.GetFloat() );
               break;
            }
            case AVP_TYPE_FLOAT64: {
               if (!value.IsDouble()) THROW_DATATYPE_MISMATCH();
               avp.set( value.GetDouble() );
               break;
            }
            default:
            {
               THROW_DATATYPE_MISMATCH();
            }
         }

         avp.addTo( reference );

         break;
      }
      case RAPIDJSON_NAMESPACE::kStringType:
      {
         if ( avp.getBaseType() != AVP_TYPE_OCTETSTRING )
            THROW_DATATYPE_MISMATCH();
         
         size_t rawlen = strlen( value.GetString() );
         sSS ss;
         char abuf[18];

         if ( avp.isAddress() )
         {
            if (inet_pton(AF_INET,value.GetString(),&((sSA4*)&ss)->sin_addr) == 1)
            {
printf("AF_INET\n");
               *(uint16_t *)abuf = htons(1);
               memcpy(abuf + 2, &((sSA4*)&ss)->sin_addr.s_addr, 4);
               avp.set( (uint8_t*)abuf, 6 );
            }
            else if (inet_pton(AF_INET6,value.GetString(),&((sSA6*)&ss)->sin6_addr) == 1)
            {
printf("AF_INET6\n");
               *(uint16_t *)abuf = htons(2);
               memcpy(abuf + 2, &((sSA6*)&ss)->sin6_addr.s6_addr, 16);
               avp.set( (uint8_t*)abuf, 18 );
            }
            else
            {
printf("UNKNOWN\n");
               avp.set( (uint8_t*)value.GetString(), rawlen );
            }
         }
         else if ( avp.isDerived() )
         {
            avp.set( (uint8_t*)value.GetString(), rawlen );
         }
         else
         {
            size_t binlen = rawlen / 2;

            /* make sure that string length is divisible by 2 (2 nibbles per byte) */
            if (rawlen % 2 != 0)
               throw runtimeInfo(
                  string_format("%s:%d - INFO - Length of HEX string not divisible by 2",
                  __FILE__, __LINE__)
               );

            /* allocate space for the binary string */
            avp.allocBuffer( binlen );

            /* validate that the string only contains hex digits */
            uint8_t *p = (uint8_t*)value.GetString();
            for (int i = 0; i < rawlen; i++)
               if ( !ISHEXDIGIT(p[i]) )
                  throw runtimeInfo(
                     string_format("%s:%d - INFO - invalid character [%c] found in string of hex digits",
                     __FILE__, __LINE__, (char)p[i])
                  );

            /* create the binary string from the hex digit string */
            for (int i = 0; i < binlen; i++)
               avp.getBuffer().get()[i] = (HEX2BIN(p[i * 2] ) << 4) + HEX2BIN(p[i * 2 + 1]);

            /* assign the string to the avp */
            avp.set( avp.getBuffer().get(), binlen );
         }

         /* add to the message/grouped avp */
         avp.addTo( reference );

         break;
      }
      case RAPIDJSON_NAMESPACE::kArrayType:
      {
         /* iterate through array elements adding them to reference */
         for (RAPIDJSON_NAMESPACE::Value::ConstValueIterator it = value.Begin();
              it != value.End();
              ++it)
         {
            fdJsonAddAvp( reference, name, *it, errfunc );
         }
         break;
      }
      case RAPIDJSON_NAMESPACE::kObjectType:
      {
         avp.addTo( reference );
         fdJsonAddAvps( avp, value, errfunc );
         break;
      }
      default:
      {
         throw runtimeInfo(
            string_format("%s:%d - INFO - invalid JSON data type encountered in element [%s]",
            __FILE__, __LINE__, name)
         );
         break;
      }
   }
}

static void fdJsonAddAvps( AVP &avp, const RAPIDJSON_NAMESPACE::Value &element, void (*errfunc)(const char*) )
{
   fdJsonAddAvps( avp.getAvp(), element, errfunc );
}

static void fdJsonAddAvps( msg_or_avp *reference, const RAPIDJSON_NAMESPACE::Value &element, void (*errfunc)(const char*) )
{
   /* iterate through each of the child elements adding them to the reference */
   for (RAPIDJSON_NAMESPACE::Value::ConstMemberIterator it = element.MemberBegin();
        it != element.MemberEnd();
        ++it)
   {
      try
      {
         fdJsonAddAvp( reference, it->name.GetString(), it->value, errfunc );
      }
      catch (runtimeInfo &exi)
      {
         errfunc( exi.what() );
      }
   }
}

int fdJsonAddAvps( const char *json, struct msg *msg, void (*errfunc)(const char*) )
{
   int ret = FDJSON_SUCCESS;
   RAPIDJSON_NAMESPACE::Document doc;

   if (doc.Parse<RAPIDJSON_NAMESPACE::kParseNoFlags>(json).HasParseError()) {
      errfunc( string_format("%s:%d - ERROR - Error parsing JSON string", __FILE__, __LINE__).c_str() );
      return FDJSON_JSON_PARSING_ERROR;
   }

   try
   {
      for (RAPIDJSON_NAMESPACE::Value::ConstMemberIterator it = doc.MemberBegin();
           it != doc.MemberEnd();
           ++it)
      {
         fdJsonAddAvp( msg, it->name.GetString(), it->value, errfunc );
      }
   }
   catch (runtimeError &ex)
   {
      errfunc( ex.what() );
      ret = FDJSON_EXCEPTION;
   }

   return ret;
}
