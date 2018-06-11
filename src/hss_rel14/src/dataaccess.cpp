/*
* Copyright (c) 2017 Sprint
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <iostream>
#include <inttypes.h>
#include <iomanip>

#include "dataaccess.h"
#include "sutility.h"
#include "serror.h"
#include "common_def.h"

#include "util.h"

extern "C" {
   #include "auc.h"
}

#define ASCII_TO_BINARY(c) \
( \
  c >= '0' && c <= '9' ? c - '0' : \
  c >= 'a' && c <= 'f' ? c - 'a' + 10 : \
  c >= 'A' && c <= 'F' ? c - 'A' + 10 : 0 \
)

#define KEY_LENGTH  (16)

void convert_ascii_to_binary(unsigned char *dest, unsigned char *src, int length)
{
  int i;
  for (i = 0; i < length; i++)
    dest[i] = (ASCII_TO_BINARY(src[i << 1]) << 4) | ASCII_TO_BINARY(src[(i << 1) + 1]);
}

#define GET_EVENT_DATA( _row, _col, _dest ) \
{ \
	SCassValue val = _row.getColumn( #_col ); \
	if ( !val.isNull() && !val.get( _dest ) ) \
		throw DAException( \
				SUtility::string_format("DataAccess::%s - ERROR - Error %d getting [%s]", \
						__func__, future.errorCode(), #_col ) \
		); \
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DAEventIdList::DAEventIdList()
{
}

DAEventIdList::~DAEventIdList()
{
   DAEventIdList::iterator it;

   while ( (it = begin()) != end() )
   {
      delete *it;
      pop_front();
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DAEventList::DAEventList()
{
}

DAEventList::~DAEventList()
{
   DAEventList::iterator it;

   while ( (it = begin()) != end() )
   {
      delete *it;
      pop_front();
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DataAccess::DataAccess()
{
}

DataAccess::~DataAccess()
{
	disconnect();
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// PUBLIC METHODS /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DataAccess::connect( const std::string &hst, const std::string &ks )
{
	host( hst );
	keyspace( ks );
	connect();
}

void DataAccess::connect( const char *hst, const char *ks )
{
	host( hst );
	keyspace( ks );
	connect();
}

void DataAccess::connect()
{
	SCassFuture connect_future = m_db.connect();

	connect_future.wait();

	if ( connect_future.errorCode() != CASS_OK )
	{
		throw DAException(
				SUtility::string_format( "DataAccess::%s - Unable to connect to %s - error_code=%d",
						__func__, m_db.host().c_str(), connect_future.errorCode() )
		);
	}
}

void DataAccess::disconnect()
{
	m_db.disconnect();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::addEvent( DAEvent &event )
{
	std::stringstream ss;

	// insert the event
	{
	   ss << "INSERT INTO events ("
	         << "scef_id, scef_ref_id, msisdn, extid, monitoring_event_configuration, user_identifier, monitoring_type"
	      << ") VALUES ("
	         << "'" << event.scef_id << "',"
	         << event.scef_ref_id << ","
	         << event.msisdn << ","
	         << "'" << event.extid << "',"
	         << "'" << event.mec_json << "',"
	         << "'" << event.ui_json << "',"
	         << event.monitoring_type
	      << ")";

	   SCassStatement stmt( ss.str().c_str() );

	   SCassFuture future = m_db.execute( stmt );

	   if ( future.errorCode() != CASS_OK )
	   {
	      throw DAException(
	         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
	         __func__, future.errorCode(), ss.str().c_str() )
	      );
	   }
	}

	// insert into events_msisdn
	if ( event.msisdn != 0 )
	{
	   ss.str( std::string() );

      ss << "INSERT INTO events_msisdn ("
            << "msisdn, scef_id, scef_ref_id"
         << ") VALUES ("
            << event.msisdn << ","
            << "'" << event.scef_id << "',"
            << event.scef_ref_id << ""
         << ")";

      SCassStatement stmt( ss.str().c_str() );

      SCassFuture future = m_db.execute( stmt );

      if ( future.errorCode() != CASS_OK )
      {
         throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
            __func__, future.errorCode(), ss.str().c_str() )
         );
      }
	}

   // insert into events_extid
   if ( !event.extid.empty() )
   {
      ss.str( std::string() );

      ss << "INSERT INTO events_extid ("
            << "extid, scef_id, scef_ref_id"
         << ") VALUES ("
            << "'" << event.extid << "',"
            << "'" << event.scef_id << "',"
            << event.scef_ref_id << ""
         << ")";

      SCassStatement stmt( ss.str().c_str() );

      SCassFuture future = m_db.execute( stmt );

      if ( future.errorCode() != CASS_OK )
      {
         throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
            __func__, future.errorCode(), ss.str().c_str() )
         );
      }
   }

	return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::getEvent( const char *scef_id, uint32_t scef_ref_id, DAEvent &event )
{
   std::stringstream ss;

   ss << "SELECT * FROM events WHERE "
      << "scef_id = '" << scef_id << "' "
      << "AND scef_ref_id = " << scef_ref_id;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK ) {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
   {
      event.init();

      GET_EVENT_DATA ( row, scef_id, event.scef_id );
      GET_EVENT_DATA ( row, scef_ref_id, event.scef_ref_id );
      GET_EVENT_DATA ( row, msisdn, event.msisdn );
      GET_EVENT_DATA ( row, extid, event.extid );
      GET_EVENT_DATA ( row, monitoring_event_configuration, event.mec_json );
      GET_EVENT_DATA ( row, monitoring_type, event.monitoring_type );
      GET_EVENT_DATA ( row, user_identifier, event.ui_json );
      return true;
   }

   return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DataAccess::deleteEvent( const char *scef_id, uint32_t scef_ref_id )
{
   DAEvent event;
   std::stringstream ss;

   if ( !getEvent( scef_id, scef_ref_id, event ) )
      return;

   {
      ss << "DELETE FROM events WHERE "
         << "scef_id = '" <<  scef_id << "' "
         << "AND scef_ref_id = " << scef_ref_id;

      SCassStatement stmt( ss.str().c_str() );

      SCassFuture future = m_db.execute( stmt );

      if ( future.errorCode() != CASS_OK ) {
         throw DAException(
               SUtility::string_format( "DataAccess::%s -Error %d executing [%s]",
                     __func__, future.errorCode(), ss.str().c_str() )
         );
      }
   }

   if ( event.msisdn != 0 )
   {
      ss.str( std::string() );

      ss << "DELETE FROM events_msisdn WHERE "
         << "msisdn=" <<  event.msisdn << " "
         << "AND scef_id='" <<  scef_id << "' "
         << "AND scef_ref_id=" << scef_ref_id;

      SCassStatement stmt( ss.str().c_str() );

      SCassFuture future = m_db.execute( stmt );

      if ( future.errorCode() != CASS_OK ) {
         throw DAException(
               SUtility::string_format( "DataAccess::%s -Error %d executing [%s]",
                     __func__, future.errorCode(), ss.str().c_str() )
         );
      }
   }

   if ( !event.extid.empty() )
   {
      ss.str( std::string() );

      ss << "DELETE FROM events_extid WHERE "
         << "extid=" <<  event.extid << " "
         << "AND scef_id='" <<  scef_id << "' "
         << "AND scef_ref_id=" << scef_ref_id;

      SCassStatement stmt( ss.str().c_str() );

      SCassFuture future = m_db.execute( stmt );

      if ( future.errorCode() != CASS_OK ) {
         throw DAException(
               SUtility::string_format( "DataAccess::%s -Error %d executing [%s]",
                     __func__, future.errorCode(), ss.str().c_str() )
         );
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::checkMSISDNExists( int64_t msisdn )
{
   std::stringstream ss;

   ss << "SELECT * FROM msisdn_imsi WHERE msisdn=" << msisdn << ";" ;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
      return true;

   return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::checkImsiExists( const char *imsi )
{
   std::stringstream ss;

   ss << "SELECT imsi FROM users_imsi WHERE imsi='" << imsi << "';";

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
      return true;

   return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::checkExtIdExists( const char *extid )
{
   std::stringstream ss;

   ss << "SELECT extid FROM extid WHERE extid = '" << extid << "';" ;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
      return true;

   return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::getImsiListFromExtId( const char *extid, DAImsiList &imsilst)
{
   std::stringstream ss;

   ss << "SELECT imsi FROM extid_imsi WHERE extid ='" << extid << "'";

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassIterator rows = res.rows();

   std::string imsi;

   while ( rows.nextRow() )
   {
      SCassRow row = rows.row();
      GET_EVENT_DATA( row, imsi, imsi );
      imsilst.push_back( imsi );
   }

   return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::getExtIdsFromImsi( const char *imsi, DAExtIdList &extids )
{
   std::stringstream ss;

   ss << "SELECT extid FROM extid_imsi_xref WHERE imsi = '" << imsi << "'";

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassIterator rows = res.rows();

   std::string extid;

   while ( rows.nextRow() )
   {
      SCassRow row = rows.row();
      GET_EVENT_DATA( row, extid, extid );
      extids.push_back( extid );
   }

   return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::getImsiFromMsisdn( int64_t msisdn, std::string &imsi )
{
   std::stringstream ss;

   ss << "SELECT imsi FROM msisdn_imsi WHERE msisdn = " << msisdn;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK ) {
      throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
                  __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() ) {
      GET_EVENT_DATA( row, imsi, imsi );
      return true;
   }

   return false;
}

bool DataAccess::getImsiFromMsisdn( const char *msisdn, std::string &imsi )
{
   std::stringstream ss;

   ss << "SELECT imsi FROM msisdn_imsi WHERE msisdn = " << msisdn;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK ) {
      throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
                  __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() ) {
      GET_EVENT_DATA( row, imsi, imsi );
      return true;
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

bool DataAccess::getMsisdnFromImsi( const char *imsi, std::string &msisdn )
{
	std::stringstream ss;

	ss << "SELECT msisdn FROM users_imsi where imsi = '"
	   << imsi << "' ;" ;

	SCassStatement stmt( ss.str().c_str() );

	SCassFuture future = m_db.execute( stmt );

	if ( future.errorCode() != CASS_OK ){
		throw DAException(
			  SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
					  __func__, future.errorCode(), ss.str().c_str() )
		);
	}

	SCassResult res = future.result();

	SCassRow row = res.firstRow();

	if ( row.valid() )
	{
		GET_EVENT_DATA( row, msisdn, msisdn );
		return true;
	}

	return false;

}

bool DataAccess::getMsisdnFromImsi( const char *imsi, int64_t &msisdn )
{
   std::stringstream ss;

   ss << "SELECT msisdn FROM users_imsi where imsi = '"
      << imsi << "' ;" ;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK ){
      throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
                 __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
   {
      GET_EVENT_DATA( row, msisdn, msisdn );
      return true;
   }

   return false;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DataAccess::getImsiInfo ( const char *imsi, DAImsiInfo &info )
{
   std::stringstream ss;

   ss << "SELECT imsi, mmehost, mmerealm, ms_ps_status, subscription_data, msisdn, visited_plmnid, access_restriction, mmeidentity_idmmeidentity FROM users_imsi where imsi = '"
      << imsi << "' ;" ;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK ) {
      throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
                  __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
   {
      GET_EVENT_DATA( row, imsi, info.imsi );
      GET_EVENT_DATA( row, mmehost, info.mmehost );
      GET_EVENT_DATA( row, mmerealm, info.mmerealm );
      GET_EVENT_DATA( row, ms_ps_status, info.ms_ps_status );
      GET_EVENT_DATA( row, subscription_data, info.subscription_data );
      GET_EVENT_DATA( row, msisdn, info.msisdn );
      info.str_msisdn = std::to_string( info.msisdn );
      GET_EVENT_DATA( row, visited_plmnid, info.visited_plmnid );
      GET_EVENT_DATA( row, access_restriction, info.access_restriction );
      GET_EVENT_DATA( row, mmeidentity_idmmeidentity, info.mme_id );
      return true;
   }

   return false;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DataAccess::getEventIdsFromMsisdn( int64_t msisdn, DAEventIdList &eil )
{
   std::stringstream ss;

   ss << "SELECT scef_id, scef_ref_id FROM events_msisdn WHERE msisdn = " << msisdn;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassIterator rows = res.rows();

   while ( rows.nextRow() )
   {
      SCassRow row = rows.row();
      DAEventId *ei = new DAEventId();

      GET_EVENT_DATA( row, scef_id, ei->scef_id );
      GET_EVENT_DATA( row, scef_ref_id, ei->scef_ref_id );

      eil.push_back( ei );
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DataAccess::getEventIdsFromExtId( const char *extid, DAEventIdList &eil )
{
   std::stringstream ss;

   ss << "SELECT scef_id, scef_ref_id FROM events_extid WHERE extid = '" << extid << "'";

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassIterator rows = res.rows();

   while ( rows.nextRow() )
   {
      SCassRow row = rows.row();
      DAEventId *ei = new DAEventId();

      GET_EVENT_DATA( row, scef_id, ei->scef_id );
      GET_EVENT_DATA( row, scef_ref_id, ei->scef_ref_id );

      eil.push_back( ei );
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DataAccess::getEventsFromImsi( const char *imsi, DAEventList &el )
{
   DAImsiInfo info;
   DAExtIdList extIdLst;
   DAEventIdList evtIdLst;

   if ( !getImsiInfo( imsi, info ) )
      return;

   getExtIdsFromImsi( imsi, extIdLst );

   // get the EventId's associated with the msisdn
   getEventIdsFromMsisdn( info.msisdn, evtIdLst );

   // get the EventId's associated with the external identifiers
   for ( DAExtIdList::iterator it = extIdLst.begin(); it != extIdLst.end(); ++it )
   {
      getEventIdsFromExtId( *it, evtIdLst );
   }

   // get the events associated with the event id's
   for ( DAEventIdList::iterator it = evtIdLst.begin(); it != evtIdLst.end(); ++it )
   {
      DAEvent *e = new DAEvent();

      if ( getEvent( (*it)->scef_id, (*it)->scef_ref_id, *e ) )
         el.push_back( e );
      else
         delete e;
   }
}

bool DataAccess::checkOpcKeys( const uint8_t opP[16] )
{
   //statement = cass_statement_new("SELECT imsi,key,OPc from vhss.users_imsi",0);
   SCassStatement stmt( "SELECT imsi,key,OPc from vhss.users_imsi" );
   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
       throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
           __func__, future.errorCode(), "SELECT imsi,key,OPc from vhss.users_imsi" )
       );
   }
   SCassResult res = future.result();
   SCassIterator rows = res.rows();

   std::cout << std::endl << std::endl;

   while ( rows.nextRow() )
   {
      SCassRow row = rows.row();

      std::string imsi;
      std::string key;
      std::string opc;

      GET_EVENT_DATA( row, imsi, imsi );
      GET_EVENT_DATA( row, key, key );
      GET_EVENT_DATA( row, OPc, opc );

      std::cout << "******************************************" << std::endl;
      std::cout << "IMSI:" << imsi << std::endl;
      std::cout << "KEY :" << key << std::endl;
      std::cout << "OPC :" << opc << std::endl;

      uint8_t opccalc[16];
      uint8_t key_bin[16];
      convert_ascii_to_binary (key_bin, (uint8_t *)key.c_str(), KEY_LENGTH);
      ComputeOPc (key_bin, opP, opccalc);

      std::stringstream ss;
      ss << std::hex << std::setfill('0') << std::setw(2);
      for(int i(0);i<KEY_LENGTH;++i){
          ss<< (unsigned int)opccalc[i];
      }
      std::string newopc = ss.str();
      std::cout << "NEW OPC :" << newopc << std::endl;
      std::cout << "******************************************" << std::endl;

      if( !updateOpc(imsi, newopc) ) {
         return false;
      }

   }

   return true;
}

bool DataAccess::updateOpc ( std::string &imsi, std::string& opc )
{

   std::stringstream ss;
   ss << "UPDATE vhss.users_imsi SET OPc='" << opc << "' WHERE imsi='" << imsi << "';";
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
      throw DAException(
         SUtility::string_format( "DataAcces::%s - Error %d executing [%s]",
               __func__, future.errorCode(), ss.str().c_str() )
      );

   return true;

}

bool DataAccess::purgeUE ( std::string &imsi )
{
   if (imsi.empty())
      return false;

   std::stringstream ss;
   ss << "UPDATE vhss.users_imsi SET ms_ps_status='PURGED' WHERE imsi='" << imsi << "';";
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
      throw DAException(
         SUtility::string_format( "DataAcces::%s - Error %d executing [%s]",
               __func__, future.errorCode(), ss.str().c_str() )
      );

   return true;
}


bool DataAccess::getMmeIdentityFromImsi ( std::string &imsi, DAMmeIdentity& mmeid)
{
   std::stringstream ss;

   ss << "SELECT mmeidentity_idmmeidentity FROM vhss.users_imsi WHERE imsi = '"
      << imsi << "';" ;
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
       throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
           __func__, future.errorCode(), ss.str().c_str() )
       );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
   {
      int32_t id;
      GET_EVENT_DATA( row, mmeidentity_idmmeidentity, id );
      return getMmeIdentity( id, mmeid );
   }

   return false;
}

bool DataAccess::getMmeIdentity ( std::string &mme_id, DAMmeIdentity& mmeid )
{
   std::stringstream ss;

   ss << "SELECT mmehost,mmerealm,mmeisdn FROM vhss.mmeidentity WHERE idmmeidentity='"
      << mme_id << "';" ;
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
       throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
           __func__, future.errorCode(), ss.str().c_str() )
       );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
   {
       GET_EVENT_DATA( row, mmehost, mmeid.mme_host );
       GET_EVENT_DATA( row, mmerealm, mmeid.mme_realm );
       GET_EVENT_DATA( row, mmeisdn, mmeid.mme_isdn );
       return true;
   }

   return false;

}

bool DataAccess::getMmeIdentity ( int32_t mme_id, DAMmeIdentity& mmeid )
{
   std::stringstream ss;

   ss << "SELECT mmehost,mmerealm,mmeisdn FROM vhss.mmeidentity WHERE idmmeidentity="
      << mme_id << ";" ;
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
       throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
           __func__, future.errorCode(), ss.str().c_str() )
       );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
   {
       GET_EVENT_DATA( row, mmehost, mmeid.mme_host );
       GET_EVENT_DATA( row, mmerealm, mmeid.mme_realm );
       GET_EVENT_DATA( row, mmeisdn, mmeid.mme_isdn );
       return true;
   }

   return false;

}


bool DataAccess::getLatestIdentity(std::string& table_name, int64_t &id)
{
   std::stringstream ss;

   ss << "SELECT id from vhss.global_ids WHERE table_name='"
      << table_name << "';" ;
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
       throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
           __func__, future.errorCode(), ss.str().c_str() )
       );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();


   if ( row.valid() )
   {
       GET_EVENT_DATA( row, id, id );
       return true;
   }

   return false;
}

bool DataAccess::updateLatestIdentity(std::string& table_name)
{
   //update vhss.global_ids set id=id+1 where table_name='%s'; ", table_name"

   std::stringstream ss;
   ss << "UPDATE vhss.global_ids set id=id+1 where table_name='" << table_name << "';";
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
      throw DAException(
         SUtility::string_format( "DataAcces::%s - Error %d executing [%s]",
               __func__, future.errorCode(), ss.str().c_str() )
      );

   return true;

}

bool DataAccess::getMmeIdFromHost ( std::string& host, int32_t &mmeid)
{
   std::stringstream ss;

   ss << "SELECT idmmeidentity FROM vhss.mmeidentity_host WHERE mmehost='"
      << host << "';" ;
   std::cout  << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
       throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
           __func__, future.errorCode(), ss.str().c_str() )
       );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();


   if ( row.valid() )
   {
       GET_EVENT_DATA( row, idmmeidentity, mmeid );
       return true;
   }

   return false;
}

bool DataAccess::addMmeIdentity(std::string &host, std::string &realm, int32_t &mmeid )
{

   {
      std::stringstream ss;
      ss << "INSERT INTO vhss.mmeidentity ("
            << "mmehost, mmerealm, idmmeidentity"
         << ") VALUES ("
            << "'" << host  << "',"
            << "'" << realm << "',"
                   << mmeid
         << ");";

      std::cout  << ss.str() << std::endl;
      SCassStatement stmt( ss.str().c_str() );

      SCassFuture future = m_db.execute( stmt );

      if ( future.errorCode() != CASS_OK )
      {
         throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
            __func__, future.errorCode(), ss.str().c_str() )
         );
      }
   }

   {
      std::stringstream ss;
      ss << "INSERT INTO vhss.mmeidentity_host ("
            << "mmehost, idmmeidentity, mmerealm"
         << ") VALUES ("
            << "'" << host  << "',"
                   << mmeid << ","
            << "'" << realm << "'"
         << ");";

      std::cout  << ss.str() << std::endl;
      SCassStatement stmt( ss.str().c_str() );

      SCassFuture future = m_db.execute( stmt );

      if ( future.errorCode() != CASS_OK )
      {
         throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
            __func__, future.errorCode(), ss.str().c_str() )
         );
      }
   }

   return true;
}

bool DataAccess::updateLocation ( DAImsiInfo &location, uint32_t present_flags )
{
   std::string table_name("vhss.mmeidentity");
   int64_t current_id(0);
   int32_t idmmeidentity(0);
   int64_t new_id(0);

   getLatestIdentity(table_name, current_id);

   if( !getMmeIdFromHost(location.mmehost, idmmeidentity ))
   {
      do
      {
         current_id++;
         getLatestIdentity(table_name, new_id);
      }
      while(current_id - 1 != new_id);

      if( !updateLatestIdentity(table_name) )
         return false;

      idmmeidentity=current_id;

      if( !addMmeIdentity(location.mmehost, location.mmerealm, idmmeidentity) )
         return false;
   }

   std::stringstream ss;
   ss << "UPDATE vhss.users_imsi SET ";

   if ( FLAG_IS_SET( present_flags, IMEI_PRESENT) ) {
      ss << "imei='" << location.imei << "',";
   }

   if ( FLAG_IS_SET( present_flags, SV_PRESENT) ) {
      ss << "imeisv='" << location.imei_sv << "',";
   }

   if ( FLAG_IS_SET( present_flags, MME_IDENTITY_PRESENT ) ) {
      ss << "mmeidentity_idmmeidentity=" << idmmeidentity << ",";
      ss << "mmehost='" << location.mmehost << "',";
      ss << "mmerealm='" << location.mmerealm << "',";
   }

   ss << "ms_ps_status='" << "ATTACHED" << "',";
   ss << "visited_plmnid='" << location.visited_plmnid << "' ";

   ss << "WHERE imsi='" << location.imsi << "';";

   std::cout << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
      throw DAException(
         SUtility::string_format( "DataAcces::%s - Error %d executing [%s]",
               __func__, future.errorCode(), ss.str().c_str() )
      );

   return true;
}

bool DataAccess::getImsiSec ( const std::string &imsi, DAImsiSec &imsisec )
{
   //SELECT key,sqn,rand,OPc FROM vhss.users_imsi WHERE imsi='%s'

   std::stringstream ss;

   ss << "SELECT key,sqn,rand,OPc FROM vhss.users_imsi WHERE imsi='"
      << imsi << "';" ;

   std::cout << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
       throw DAException(
           SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
           __func__, future.errorCode(), ss.str().c_str() )
       );
   }

   SCassResult res = future.result();

   SCassRow row = res.firstRow();

   if ( row.valid() )
   {
      std::string key_str;
      int64_t     sqn_nb;
      std::string rand_str;
      std::string OPc_str;

      GET_EVENT_DATA( row, key, key_str );
      GET_EVENT_DATA( row, sqn, sqn_nb );
      GET_EVENT_DATA( row, rand, rand_str );
      GET_EVENT_DATA( row, OPc, OPc_str );

      convert_ascii_to_binary(imsisec.key, (uint8_t *)key_str.c_str(), KEY_LENGTH);
      convert_ascii_to_binary(imsisec.rand,(uint8_t *)rand_str.c_str(), RAND_LENGTH);
      convert_ascii_to_binary(imsisec.opc, (uint8_t *)OPc_str.c_str(), KEY_LENGTH);

      imsisec.sqn[0] = (sqn_nb & (255UL << 40)) >> 40;
      imsisec.sqn[1] = (sqn_nb & (255UL << 32)) >> 32;
      imsisec.sqn[2] = (sqn_nb & (255UL << 24)) >> 24;
      imsisec.sqn[3] = (sqn_nb & (255UL << 16)) >> 16;
      imsisec.sqn[4] = (sqn_nb & (255UL << 8)) >> 8;
      imsisec.sqn[5] = (sqn_nb & 0xFF);

      return true;
   }

   return false;
}

bool DataAccess::updateRandSqn ( const std::string &imsi, uint8_t * rand_p, uint8_t * sqn )
{
   SqnU64Union eu;

   SQN_TO_U64(sqn, eu);

   std::string rand = Utility::bytes2hex(rand_p, RAND_LENGTH);

   std::cout << "sqn=" << Utility::bytes2hex(sqn,6,'.') << " eu.u8[]=" << Utility::bytes2hex(eu.u8,8,'.') << " eu.u64=" << eu.u64 << " rand=[" << rand << "]" << std::endl;

   std::stringstream ss;
   ss << "UPDATE vhss.users_imsi SET rand='" << rand << "', sqn=" << eu.u64 << " WHERE imsi='" << imsi << "';";
   std::cout << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
      throw DAException(
         SUtility::string_format( "DataAcces::%s - Error %d executing [%s]",
               __func__, future.errorCode(), ss.str().c_str() )
      );

   return true;
}

bool DataAccess::incSqn ( std::string &imsi, uint8_t * sqn )
{
   SqnU64Union eu;

   SQN_TO_U64(sqn, eu);

   eu.u64 += 32;

   std::stringstream ss;
   ss << "UPDATE vhss.users_imsi SET sqn =" << eu.u64 << " WHERE imsi='" << imsi << "';";
   std::cout << ss.str() << std::endl;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
      throw DAException(
         SUtility::string_format( "DataAcces::%s - Error %d executing [%s]",
               __func__, future.errorCode(), ss.str().c_str() )
      );

   return true;
}

bool DataAccess::getSubDataFromImsi( const char *imsi, std::string &sub_data )
{
    std::stringstream ss;

    ss << "SELECT subscription_data FROM users_imsi where imsi = '"
       << imsi << "' ;" ;

    SCassStatement stmt( ss.str().c_str() );

    SCassFuture future = m_db.execute( stmt );

    if ( future.errorCode() != CASS_OK )
    {
        throw DAException(
            SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
            __func__, future.errorCode(), ss.str().c_str() )
        );
    }

    SCassResult res = future.result();

    SCassRow row = res.firstRow();

    if ( row.valid() )
    {
        GET_EVENT_DATA( row, subscription_data, sub_data );
        return true;
    }

    return false;

}

void DataAccess::UpdateValidityTime( const char *imsi, std::string &validity_time )
{
       std::stringstream ss;

       ss << "UPDATE users_imsi SET niddvalidity = '"
          << validity_time << "' WHERE imsi= '" << imsi << "' ;" ;

       SCassStatement stmt( ss.str().c_str() );

       SCassFuture future = m_db.execute( stmt );

       if ( future.errorCode() != CASS_OK )
       {
               throw DAException(
                   SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
                   __func__, future.errorCode(), ss.str().c_str() )
               );
       }

}

void DataAccess::UpdateNIRDestination( const char *imsi, std::string &host, std::string &realm )
{
       std::stringstream ss;

       ss << "UPDATE users_imsi SET nir_dest_host = '"
          << host << "' , nir_dest_realm = '"
          << realm << "' WHERE imsi='"
          << imsi << "' ;" ;

       SCassStatement stmt( ss.str().c_str() );

       SCassFuture future = m_db.execute( stmt );

       if ( future.errorCode() != CASS_OK )
       {
               throw DAException(
                               SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
                                               __func__, future.errorCode(), ss.str().c_str() )
               );
       }

}



#if 0

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////





bool DataAccess::getImsiFromScefIdScefRefId( char *scef_id, uint32_t scef_ref_id, std::string &imsi )
{
	std::stringstream ss;

	ss << "SELECT imsi FROM scef_ids WHERE scef_id = '"
	   <<  scef_id
	   << "' AND scef_ref_id = " << scef_ref_id <<";" ;

	SCassStatement stmt( ss.str().c_str() );

	SCassFuture future = m_db.execute( stmt );

	if( future.errorCode() != CASS_OK ) {
		throw DAException(
				SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
						__func__, future.errorCode(), ss.str().c_str() )
		);
	}

	SCassResult res = future.result();

	SCassRow row = res.firstRow();

	if ( row.valid() ) {
		GET_EVENT_DATA( row, imsi, imsi);
		return true;
	}

	return false;
}

bool DataAccess::getScefIdScefRefIdFromImsi( char *imsi, std::string &scef_id, uint32_t &scef_ref_id )
{
	std::stringstream ss;

	ss << "SELECT scef_id, scef_ref_id FROM scef_ids_imsi WHERE imsi = '" << imsi << "' ;" ;

	SCassStatement stmt( ss.str().c_str() );

	SCassFuture future = m_db.execute( stmt );

	if ( future.errorCode() != CASS_OK ) {
		throw DAException(
				SUtility::string_format( "DataAccess::%s -Error %d executing [%s]",
						__func__, future.errorCode(), ss.str().c_str() )
		);
	}

	SCassResult res = future.result();

	SCassRow row = res.firstRow();

	if ( row.valid() ) {
		GET_EVENT_DATA( row, scef_id, scef_id);
		GET_EVENT_DATA( row, scef_ref_id, scef_ref_id);
		return true;
	}

	return false;
}

bool DataAccess::deleteEvent( std::string scef_id, uint32_t scef_ref_id )
{
	std::stringstream ss;

	ss << "DELETE FROM monitoring_event_configuration WHERE scef_id = '"
	   <<  scef_id
	   << "' AND scef_ref_id = " << scef_ref_id << " ;" ;

	SCassStatement stmt( ss.str().c_str() );

	SCassFuture future = m_db.execute( stmt );

	if ( future.errorCode() != CASS_OK ) {
		throw DAException(
				SUtility::string_format( "DataAccess::%s -Error %d executing [%s]",
						__func__, future.errorCode(), ss.str().c_str() )
		);
	}

	return true;
}

bool DataAccess::delScefIds( const char *imsi )
{
	std::stringstream ss;

	ss << "DELETE FROM scef_ids_imsi WHERE imsi = '" << imsi << "' ;" ;

	SCassStatement stmt( ss.str().c_str() );

	SCassFuture future = m_db.execute( stmt );

	if ( future.errorCode() != CASS_OK ) {
		throw DAException(
				SUtility::string_format( "DataAccess::%s -Error %d executing [%s]",
						__func__, future.errorCode(), ss.str().c_str() )
		);
	}

	return true;
}

bool DataAccess::delImsiFromScefIds ( const char *scef_id, uint32_t scef_ref_id )
{
	std::stringstream ss;

	ss << "DELETE FROM scef_ids WHERE scef_id = '" << scef_id
	   << "' AND scef_ref_id = " << scef_ref_id << " ;" ;

	SCassStatement stmt( ss.str().c_str() );

	SCassFuture future = m_db.execute( stmt );

	if ( future.errorCode() != CASS_OK ) {
		throw DAException(
				SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
						__func__, future.errorCode(), ss.str().c_str() )
		);
	}

	return true;
}

bool DataAccess::CheckImsiAttached ( char *imsi )
{
	std::stringstream ss;

	ss << "SELECT ms_ps_status FROM users_imsi WHERE imsi = '" << imsi << "' ;" ;

	SCassStatement stmt( ss.str().c_str() );

	SCassFuture future = m_db.execute( stmt );

	if ( future.errorCode() != CASS_OK ) {
		throw DAException(
				SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
						__func__, future.errorCode(), ss.str().c_str() )
		);
	}

	SCassResult res = future.result();

	SCassRow row = res.firstRow();

	std::string imsi_status;

	if ( row.valid() )
	{
		GET_EVENT_DATA( row, ms_ps_status, imsi_status);
	}

	return imsi_status.compare( "ATTACHED" ) == 0;
}

void DataAccess::getMonEventConfForImsi( const char *imsi, DAMonitoringConfEventList &mcel )
{
   std::stringstream ss;

   ss << "SELECT imsi FROM extidentifier_imsi WHERE extid ='" << ext_id << "';" ;

   SCassStatement stmt( ss.str().c_str() );

   SCassFuture future = m_db.execute( stmt );

   if ( future.errorCode() != CASS_OK )
   {
      throw DAException(
         SUtility::string_format( "DataAccess::%s - Error %d executing [%s]",
         __func__, future.errorCode(), ss.str().c_str() )
      );
   }

   SCassResult res = future.result();

   SCassIterator rows = res.rows();

   std::string imsi;

   while ( rows.nextRow() ) {
      SCassRow row = rows.row();
      GET_EVENT_DATA( row, imsi, imsi );
      imsilst.push_back( imsi );
   }

   return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DAMonitoringConfEventList::DAMonitoringConfEventList()
{
}

DAMonitoringConfEventList::~DAMonitoringConfEventList()
{
   std::list<MonitoringConfEvent*>::iterator it;

   while ( (it = begin()) != end() )
   {
      delete *it;
      pop_front();
   }
}

#endif
