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

#ifndef __DATAACCESS_H_
#define __DATAACCESS_H_

#include <stdexcept>
#include <list>
#include <string>

#include "scassandra.h"

#define MME_IDENTITY_PRESENT              (1U)
#define MME_SUPPORTED_FEATURES_PRESENT    (1U << 1)
#define IMEI_PRESENT                      (1U << 2)
#define SV_PRESENT                        (1U << 3)
#define UE_SRVCC_PRESENT                  (1U << 4)

#define KEY_LENGTH  (16)
#define SQN_LENGTH  (6)
#define RAND_LENGTH (16)


class DAException : public std::runtime_error
{
public:
	DAException(const char *m) : std::runtime_error(m) {}
	DAException(const std::string &m) : std::runtime_error(m) {}
};

class DAImsiList : public std::list<std::string>
{
public:
	DAImsiList() {}
	~DAImsiList() {}
};

class DAExtIdList : public std::list<std::string>
{
public:
   DAExtIdList() {}
   ~DAExtIdList() {}
};

class DAEvent
{
public:
   DAEvent()
   {
      init();
   }

   void init()
   {
      scef_id.clear();
      scef_ref_id = 0;
      msisdn = 0;
      extid.clear();
      ui_json.clear();
      mec_json.clear();
   }

	std::string scef_id;
	uint32_t scef_ref_id;
	int64_t msisdn;
	std::string extid;
	std::string ui_json;
	std::string mec_json;
	int32_t monitoring_type;
};

class DAEventList : public std::list<DAEvent*>
{
public:
   DAEventList();
   ~DAEventList();
};

struct DAEventId
{
   std::string scef_id;
   uint32_t scef_ref_id;
};

struct DAMmeIdentity
{
   std::string mme_host;
   std::string mme_realm;
   std::string mme_isdn;
};

class DAEventIdList : public std::list<DAEventId*>
{
public:
   DAEventIdList();
   ~DAEventIdList();
};

struct DAImsiInfo {
   std::string imsi;
   std::string mmehost;
	std::string mmerealm;
	std::string ms_ps_status;
	std::string subscription_data;
	int64_t msisdn;
	std::string str_msisdn;
	std::string visited_plmnid;
	int32_t access_restriction;
	std::string imei;
	std::string imei_sv;
	int32_t mme_id;
};

struct DAImsiSec {
   uint8_t key[KEY_LENGTH];
   uint8_t sqn[SQN_LENGTH];
   uint8_t rand[RAND_LENGTH];
   uint8_t opc[KEY_LENGTH];
};


class DataAccess
{
public:
	DataAccess();
	virtual ~DataAccess();

	std::string &host( const char *hst ) { return m_db.host(hst); }
	std::string &host( const std::string &hst ) { return m_db.host(hst); }
	std::string &host() { return m_db.host(); }

	std::string &keyspace( const char *ks ) { return m_db.keyspace(ks); }
	std::string &keyspace( const std::string &ks ) { return m_db.keyspace(ks); }
	std::string &keyspace() { return m_db.keyspace(); }

	int protocolVersion( int pv ) { return m_db.protocolVersion(pv); }
	int protocolVersion() { return m_db.protocolVersion(); }

	void connect();
	void connect( const std::string &hst, const std::string &ks = "vhss" );
	void connect( const char *hst, const char *ks = "vhss" );

	void disconnect();

	bool addEvent( DAEvent &event );

   bool getEvent( const char *scef_id, uint32_t scef_ref_id, DAEvent &event );
   bool getEvent( const std::string &scef_id, uint32_t scef_ref_id, DAEvent &event ) { return getEvent( scef_id.c_str(), scef_ref_id, event ); }

   void deleteEvent( const char *scef_id, uint32_t scef_ref_id );
   void deleteEvent( const std::string &scef_id, uint32_t scef_ref_id ) { deleteEvent( scef_id.c_str(), scef_ref_id ); }

	bool checkMSISDNExists( int64_t msisdn) ;

   bool checkImsiExists( const char *imsi );
	bool checkImsiExists( const std::string &imsi ) { return checkImsiExists( imsi.c_str() ); }

   bool checkExtIdExists( const char *extid );
	bool checkExtIdExists( const std::string &extid ) { return checkExtIdExists( extid.c_str() ); }

	bool getImsiListFromExtId( const char *extid, DAImsiList &imsilst);
   bool getImsiListFromExtId( std::string &extid, DAImsiList &imsilst) { return getImsiListFromExtId( extid.c_str(), imsilst ); }

   bool getExtIdsFromImsi( const char *imsi, DAExtIdList &extids );
   bool getExtIdsFromImsi( const std::string &imsi, DAExtIdList &extids ) { return getExtIdsFromImsi( imsi.c_str(), extids ); }

	bool getImsiFromMsisdn( int64_t msisdn, std::string &imsi );
   bool getImsiFromMsisdn( const char *msisdn, std::string &imsi );
   bool getImsiFromMsisdn( const std::string &msisdn, std::string &imsi ) { return getImsiFromMsisdn( msisdn.c_str(), imsi ); }

	bool getMsisdnFromImsi( const char *imsi, std::string &msisdn );
   bool getMsisdnFromImsi( const std::string &imsi, std::string &msisdn ) { return getMsisdnFromImsi( imsi.c_str(), msisdn ); }
   bool getMsisdnFromImsi( const char *imsi, int64_t &msisdn );

   bool getImsiInfo ( const char *imsi, DAImsiInfo &info );
   bool getImsiInfo ( const std::string &imsi, DAImsiInfo &info ) { return getImsiInfo( imsi.c_str(), info ); }

   void getEventIdsFromMsisdn( int64_t msisdn, DAEventIdList &el );

   void getEventIdsFromExtId( const char *extid, DAEventIdList &el );
   void getEventIdsFromExtId( const std::string &extid, DAEventIdList &el ) { getEventIdsFromExtId( extid.c_str(), el ); }

   void getEventsFromImsi( const char *imsi, DAEventList &mcel );
   void getEventsFromImsi( const std::string &imsi, DAEventList &mcel ) { getEventsFromImsi( imsi.c_str(), mcel ); }

   bool checkOpcKeys( const uint8_t opP[16] );
   bool updateOpc ( std::string &imsi, std::string& opc );

   bool purgeUE ( std::string &imsi );

   bool getMmeIdentityFromImsi ( std::string &imsi, DAMmeIdentity& mmeid );

   bool getMmeIdentity ( std::string &mme_id, DAMmeIdentity& mmeid );
   bool getMmeIdentity ( int32_t mme_id, DAMmeIdentity& mmeid );

   bool getLatestIdentity(std::string& table_name, int64_t &id);

   bool updateLatestIdentity(std::string& table_name);

   bool getMmeIdFromHost ( std::string& host, int32_t &mmeid );

   bool addMmeIdentity(std::string &host, std::string &realm, int32_t &mmeid );

   bool updateLocation ( DAImsiInfo &location, uint32_t present_flags );

   bool getImsiSec ( const std::string &imsi, DAImsiSec &imsisec );

   bool updateRandSqn ( const std::string &imsi, uint8_t * rand_p, uint8_t * sqn );

   bool incSqn ( std::string &imsi, uint8_t * sqn );

   bool getSubDataFromImsi( const char *imsi, std::string &sub_data );
   bool getSubDataFromImsi( const std::string &imsi, std::string &sub_data )
   {
       return getSubDataFromImsi( imsi.c_str(), sub_data);
   }

   void UpdateValidityTime( const char *imsi, std::string &validity_time );
   void UpdateValidityTime( const std::string &imsi, std::string &validity_time )
   {
          UpdateValidityTime( imsi.c_str(), validity_time);
   }

   void UpdateNIRDestination( const char *imsi, std::string &host, std::string &realm );
   void UpdateNIRDestination( const std::string &imsi, std::string &host, std::string &realm )
   {
          UpdateNIRDestination( imsi.c_str(), host, realm );
   }


//	bool getImsiFromScefIdScefRefId( char *scef_id, uint32_t scef_ref_id, std::string &imsi );

//	bool getScefIdScefRefIdFromImsi( char *imsi, std::string &scef_id, uint32_t &scef_ref_id );

//	bool delScefIds( const char *imsi);

//	bool delImsiFromScefIds ( const char *scef_id, uint32_t scef_ref_id );

// bool isImsiAttached ( const char *imsi );
// bool isImsiAttached ( const std::string &imsi ) { return isImsiAttached( imsi.c_str() ); }

private:

	SCassandra m_db;
};

#endif /* __DATAACCESS_H */
