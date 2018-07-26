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

#include <string>
#include <iostream>
#include <sstream>
#include "s6as6d_impl.h"
#include "fdjson.h"
#include "options.h"
#include "common_def.h"
#include "s6t.h"
#include "s6t_impl.h"
#include "dataaccess.h"
#include "fdhss.h"
#include "rapidjson/document.h"
#include <iomanip>
extern "C" {
#include "hss_config.h"
#include "access_restriction.h"
#include "aucpp.h"
#include <inttypes.h>
}


/* ULR-Flags meaning: */
#define ULR_SINGLE_REGISTRATION_IND       (1U)
#define ULR_S6A_S6D_INDICATOR             (1U << 1)
#define ULR_SKIP_SUBSCRIBER_DATA          (1U << 2)
#define ULR_GPRS_SUBSCRIPTION_DATA_IND    (1U << 3)
#define ULR_NODE_TYPE_IND                 (1U << 4)
#define ULR_INITIAL_ATTACH_IND            (1U << 5)
#define ULR_PS_LCS_SUPPORTED_BY_UE        (1U << 6)
#define ULR_PAD_VALID(x)                  ((x & ~0x7f) == 0)

/* Access-restriction-data bits */
#define UTRAN_NOT_ALLOWED            (1U)
#define GERAN_NOT_ALLOWED            (1U << 1)
#define GAN_NOT_ALLOWED              (1U << 2)
#define I_HSDPA_EVO_NOT_ALLOWED      (1U << 3)
#define E_UTRAN_NOT_ALLOWED          (1U << 4)
#define HO_TO_NON_3GPP_NOT_ALLOWED   (1U << 5)

//#define FLAG_IS_SET(x, flag)         ((x) & (flag))


/* PUR-Flags */
#define PUR_UE_PURGED_IN_MME      (1U)
#define PUR_UE_PURGED_IN_SGSN     (1U << 1)
#define PUR_PAD_VALID(x)         ((x & ~0x3) == 0)


#define AUTH_MAX_EUTRAN_VECTORS 6


namespace s6as6d {

void fillscefnotify(DataAccess &dataaccess, DAEventIdList& event_list, uint32_t evt_type, std::vector< std::pair< std::string, uint32_t> > &scef_tonotify);

void fdJsonError( const char *err )
{
   printf("json error: %s", err);
}

// Member functions that customize the individual application
Application::Application( DataAccess &dbobj )
    : ApplicationBase()
    , m_cmd_uplr( *this )
    //, m_cmd_calr( *this )
    , m_cmd_auir( *this )
    //, m_cmd_insdr( *this )
    //, m_cmd_desdr( *this )
    , m_cmd_puur( *this )
    //, m_cmd_rer( *this )
    , m_dbobj( dbobj )
{
    registerHandlers();
}

Application::~Application()
{
}

void Application::registerHandlers()
{
    // Remove the comments for any Command this application should
    // listen for (receive).
    std::cout << "Registering s6as6d command handlers" << std::endl;
    std::cout << "Registering UPLR command handler" << std::endl;
    registerHandler( m_cmd_uplr );
    //std::cout << "Registering CALR command handler" << std::endl;
    //registerHandler( m_cmd_calr );
    std::cout << "Registering AUIR command handler" << std::endl;
    registerHandler( m_cmd_auir );
    //std::cout << "Registering INSDR command handler" << std::endl;
    //registerHandler( m_cmd_insdr );
    //std::cout << "Registering DESDR command handler" << std::endl;
    //registerHandler( m_cmd_desdr );
    std::cout << "Registering PUUR command handler" << std::endl;
    registerHandler( m_cmd_puur );
    //std::cout << "Registering RER command handler" << std::endl;
    //registerHandler( m_cmd_rer );
}

// UPLR Request (req) Command member functions


// Sends a UPLR Request to the corresponding Peer
bool Application::sendUPLRreq(FDPeer &peer)
{
    //TODO - This code may be modified based on specific
    //         processing needs to send the UPLR Command
    UPLRreq *s = createUPLRreq( peer );

    try
    {
         if ( s )
         {
              s->send();
         }
    }
    catch ( FDException &ex )
    {
         std::cout << SUtility::currentTime() << " - EXCEPTION - " << ex.what() << std::endl;
         delete s;
         s = NULL;
    }

    // DO NOT free the newly created UPLRreq object!!
    // It will be deleted by the framework after the
    // answer is received and processed.
    return s != NULL;
}

// A factory for UPLR reuqests
UPLRreq *Application::createUPLRreq(FDPeer &peer)
{
    //  creates the UPLRreq object
    UPLRreq *s = new UPLRreq( *this );

    //TODO - Code must be added to correctly
    //         populate the UPLR request object

    // return the newly created request object
    return s;
}

// A handler for Answers corresponding to this specific Request
void UPLRreq::processAnswer( FDMessageAnswer &ans )
{
// TODO - This code must be implemented IF the application
// receives Answers for this command, i.e. it sends the
// UPLR Command
}

void display_error_message(const char *err_msg)
{

}

// Function invoked when a UPLR Command is received
int UPLRcmd::process( FDMessageRequest *req )
{
	std::string             s;
	uint32_t                u32;

	uint32_t                present_flags(0);
	//cassandra_ul_ans_t      cass_ans;
	//cassandra_ul_push_t     cass_push;
	int                     result_code = ER_DIAMETER_SUCCESS;
	bool                    experimental = false;

   uint8_t                 plmn_id[4];
   size_t                  plmn_len = sizeof(plmn_id);

   std::string             current_plmn_id_str;
   DAImsiInfo              imsi_original_info;
   DAImsiInfo              imsi_new_info;


req->dump();

	FDMessageAnswer ans( req );
	ans.addOrigin();

	s6as6d::UpdateLocationRequestExtractor ulr( *req, m_app.getDict() );

	do{

       ulr.auth_session_state.get(u32);
       ans.add( m_app.getDict().avpAuthSessionState(), u32 );

	    ulr.user_name.get(imsi_new_info.imsi);
	    if(imsi_new_info.imsi.size() > IMSI_LENGTH){
	        result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	        break;
	    }

	    if(! m_app.dataaccess().getImsiInfo(imsi_new_info.imsi.c_str(), imsi_original_info)){
          experimental = true;
          result_code = DIAMETER_ERROR_USER_UNKNOWN;
          break;
	    }

	    ulr.origin_host.get(imsi_new_info.mmehost);
	    ulr.origin_realm.get(imsi_new_info.mmerealm);


	    ulr.rat_type.get(u32);
	    if(u32 != 1004 || FLAG_IS_SET(imsi_original_info.access_restriction , E_UTRAN_NOT_ALLOWED)){
	        result_code = DIAMETER_ERROR_RAT_NOT_ALLOWED;
	        break;
	    }

	    ulr.ulr_flags.get(u32);

	    if (FLAG_IS_SET (u32, ULR_SINGLE_REGISTRATION_IND)) {
	        result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	        break;
	    }
	    if (!FLAG_IS_SET (u32, ULR_S6A_S6D_INDICATOR)) {
	        result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	        break;
	    }
	    if (FLAG_IS_SET (u32, ULR_NODE_TYPE_IND)) {
	        result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	        break;
	    }

	    if (FLAG_IS_SET (u32, ULR_INITIAL_ATTACH_IND)) {
	       FLAGS_SET(present_flags, MME_IDENTITY_PRESENT);
	    }
	    else{
	        if ( ( !imsi_original_info.mmehost.empty() ) && ( !imsi_original_info.mmerealm.empty() ) ) {

	            if(imsi_original_info.mmehost != imsi_new_info.mmehost)
	            {
	                experimental = true;
	                result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
	                break;
	            }
	            if (imsi_original_info.mmerealm != imsi_new_info.mmerealm)
	            {
	                experimental = true;
	                result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
	                break;
	            }
	        }
	        else{
	            experimental = true;
	            result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
	            break;
	        }
	    }

	    if (!ULR_PAD_VALID (u32)) {
	        result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	        break;
	    }

	    if(ulr.visited_plmn_id.get(plmn_id, plmn_len)){
	        if(plmn_len != 3){
	            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	            break;
	        }
	        else{
	           //Parse the plmn_id
	           PLMNID_TO_HEX_STR(imsi_new_info.visited_plmnid, plmn_id);
	        }
	    }
	    else{
            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
            break;
	    }

	    if(ulr.terminal_information.imei.get(imsi_new_info.imei)){
	        if(imsi_new_info.imei.size() > IMEI_LENGTH){
	            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	            break;
	        }
	        FLAGS_SET(present_flags, IMEI_PRESENT);
	    }
	    if(ulr.terminal_information.software_version.get(imsi_new_info.imei_sv)){
	        if(imsi_new_info.imei_sv.size() != SV_LENGTH){
	            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
	            break;
	        }
	        FLAGS_SET(present_flags, SV_PRESENT);
	    }
	    if(ulr.terminal_information.tgpp2_meid.get(s)){
	        result_code = ER_DIAMETER_AVP_UNSUPPORTED;
	        break;
	    }

	    if(ulr.ue_srvcc_capability.get(u32)){
	        FLAGS_SET(present_flags, UE_SRVCC_PRESENT);
	    }


	    for ( std::list<SupportedFeaturesExtractor*>::iterator seit = ulr.supported_features.getList().begin();
	            seit != ulr.supported_features.getList().end();
	          ++seit )
	    {
	        (*seit)->vendor_id.get(u32);
	        if(u32 == VENDOR_3GPP){
	            (*seit)->feature_list.get(u32);
	            FLAGS_SET(present_flags, MME_SUPPORTED_FEATURES_PRESENT);
	        }
	    }

	    ///////////////////
	    ///////////////////
	    //
	       m_app.dataaccess().updateLocation(imsi_new_info, present_flags);
	    //
	    ///////////////////
	    ///////////////////

	    ans.add( m_app.getDict().avpUlaFlags(), 1 );

	    ulr.ulr_flags.get(u32);
	    if (!FLAG_IS_SET (u32, ULR_SKIP_SUBSCRIBER_DATA)) {
	        if(fdJsonAddAvps(imsi_original_info.subscription_data.c_str(), ans.getMsg(), &display_error_message) != 0){
	            result_code = DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION;
	            experimental = true;
	        }
	        else
	        {
              DAEventList el;
              m_app.dataaccess().getEventsFromImsi( imsi_new_info.imsi.c_str(), el );
              if ( !el.empty() )
              {
                 s6as6d::UpdateLocationAnswerExtractor ula( ans, m_app.getDict() );
                 FDAvp sd( m_app.getDict().avpSubscriptionData(), (struct avp *)ula.subscription_data.getReference(), false );
                 for ( DAEventList::iterator it = el.begin(); it != el.end(); ++it )
                 {
                    sd.addJson( (*it)->mec_json );
                 }
              }
	        }
	    }

	} while (false);

	//Handle errors
	if (DIAMETER_ERROR_IS_VENDOR (result_code) && experimental) {
        FDAvp experimental_result ( m_app.getDict().avpExperimentalResult() );
        experimental_result.add( m_app.getDict().avpVendorId(),  VENDOR_3GPP);
        experimental_result.add( m_app.getDict().avpExperimentalResultCode(),  result_code);
        ans.add(experimental_result);
	}
	else{
	    ans.add( m_app.getDict().avpResultCode(), result_code);
	}

ans.dump();

	ans.send();

	try{
	   //After sending the ula, we verify if we need to send a RIR reporting the change of pmnid

	   //////////////////////////////////
	   /// What if the db is out of sync
	   //////////////////////////////////
	   if( imsi_original_info.ms_ps_status == "ATTACHED"  ) {
	      //If the plmnid has changed, we check if this has to be reported
	      //imsi_original_info.visited_plmnid

	      if( imsi_original_info.visited_plmnid != imsi_new_info.visited_plmnid )
	      {
            DAEventList evt_list;
            m_app.dataaccess().getEventsFromImsi( imsi_new_info.imsi, evt_list );

            for( DAEventList::iterator it_evt = evt_list.begin() ; it_evt != evt_list.end(); ++ it_evt )
            {
               if ( (*it_evt)->monitoring_type == ROAMING_STATUS_EVT )
               {
                  //Build the RIR for each scef to be notified
                  s6t::REIRreq *s = new s6t::REIRreq( *fdHss.gets6tApp() );
                  s->add( fdHss.gets6tApp()->getDict().avpSessionId(), s->getSessionId() );
                  s->add( fdHss.gets6tApp()->getDict().avpAuthSessionState(), 1 );
                  s->addOrigin();

                  s->add ( fdHss.gets6tApp()->getDict().avpDestinationHost() , (*it_evt)->scef_id );

                  size_t afound = (*it_evt)->scef_id.find(".");
                  if(afound != std::string::npos)
                  {
                     s->add ( fdHss.gets6tApp()->getDict().avpDestinationRealm() , (*it_evt)->scef_id.substr(afound + 1) );
                  }

                  FDAvp monitoring_event_report( fdHss.gets6tApp()->getDict().avpMonitoringEventReport() );

                  monitoring_event_report.add(fdHss.gets6tApp()->getDict().avpScefId(), (*it_evt)->scef_id);
                  monitoring_event_report.add(fdHss.gets6tApp()->getDict().avpScefReferenceId(), (*it_evt)->scef_id);
                  monitoring_event_report.add(fdHss.gets6tApp()->getDict().avpMonitoringType(), ROAMING_STATUS_EVT);
                  monitoring_event_report.add(fdHss.gets6tApp()->getDict().avpVisitedPlmnId(), plmn_id, plmn_len );

                  s->add(monitoring_event_report);

s->dump();

                  try
                  {
                       if ( s )
                       {
                            s->send();
                       }
                  }
                  catch ( FDException &ex )
                  {
                       std::cout << SUtility::currentTime() << " - EXCEPTION sending - " << ex.what() << std::endl;
                       delete s;
                       s = NULL;
                  }
               }
            }
	      }
	   }
	}
	catch( ... ){
	   std::cout << "Error while generating evnt report" << std::endl;
	}

	return 0;
}

 
// CALR Request (req) Command member functions


// Sends a CALR Request to the corresponding Peer
bool Application::sendCALRreq(FDPeer &peer)
{
    //TODO - This code may be modified based on specific
    //         processing needs to send the CALR Command
    CALRreq *s = createCALRreq( peer );

    try
    {
         if ( s )
         {
              s->send();
         }
    }
    catch ( FDException &ex )
    {
         std::cout << SUtility::currentTime() << " - EXCEPTION - " << ex.what() << std::endl;
         delete s;
         s = NULL;
    }

    // DO NOT free the newly created CALRreq object!!
    // It will be deleted by the framework after the
    // answer is received and processed.
    return s != NULL;
}

// A factory for CALR reuqests
CALRreq *Application::createCALRreq(FDPeer &peer)
{
    //  creates the CALRreq object
    CALRreq *s = new CALRreq( *this );

    //TODO - Code must be added to correctly
    //         populate the CALR request object

    // return the newly created request object
    return s;
}

// A handler for Answers corresponding to this specific Request
void CALRreq::processAnswer( FDMessageAnswer &ans )
{
// TODO - This code must be implemented IF the application
// receives Answers for this command, i.e. it sends the
// CALR Command
}

// CALR Command (cmd) member function

// Function invoked when a CALR Command is received
int CALRcmd::process( FDMessageRequest *req )
{
// TODO - This code must be implemented IF the application
// receives the CALR command.
    return -1;
}
 
// AUIR Request (req) Command member functions


// Sends a AUIR Request to the corresponding Peer
bool Application::sendAUIRreq(FDPeer &peer)
{
    //TODO - This code may be modified based on specific
    //         processing needs to send the AUIR Command
    AUIRreq *s = createAUIRreq( peer );

    try
    {
         if ( s )
         {
              s->send();
         }
    }
    catch ( FDException &ex )
    {
         std::cout << SUtility::currentTime() << " - EXCEPTION - " << ex.what() << std::endl;
         delete s;
         s = NULL;
    }

    // DO NOT free the newly created AUIRreq object!!
    // It will be deleted by the framework after the
    // answer is received and processed.
    return s != NULL;
}

// A factory for AUIR reuqests
AUIRreq *Application::createAUIRreq(FDPeer &peer)
{
    //  creates the AUIRreq object
    AUIRreq *s = new AUIRreq( *this );

    //TODO - Code must be added to correctly
    //         populate the AUIR request object

    // return the newly created request object
    return s;
}

// A handler for Answers corresponding to this specific Request
void AUIRreq::processAnswer( FDMessageAnswer &ans )
{
// TODO - This code must be implemented IF the application
// receives Answers for this command, i.e. it sends the
// AUIR Command
}

// AUIR Command (cmd) member function

// Function invoked when a AUIR Command is received
int AUIRcmd::process( FDMessageRequest *req )
{

    std::string                     s;
    std::string                     imsi_str;
    uint32_t                        u32;

    DAImsiSec                       imsisec;

    auc_vector_t                    vector[AUTH_MAX_EUTRAN_VECTORS];
    int                             result_code = ER_DIAMETER_SUCCESS;
    int                             experimental = 0;
    uint64_t                        imsi = 0;

    uint8_t                         *sqn = NULL;
    uint32_t                        num_vectors = 0;

    uint8_t                         plmn_id[4];
    size_t                          plmn_len = sizeof(plmn_id);

    uint8_t                         auts[31];
    size_t                          auts_len = sizeof(auts);
    bool                            auts_set = false;


req->dump();

    FDMessageAnswer ans( req );
    ans.addOrigin();

    s6as6d::AuthenticationInformationRequestExtractor air( *req, m_app.getDict() );

    do{
        air.auth_session_state.get(u32);
        ans.add( m_app.getDict().avpAuthSessionState(), u32 );

        air.user_name.get(imsi_str);
        if(strlen(imsi_str.c_str()) > IMSI_LENGTH){
            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
            break;
        }

        sscanf (imsi_str.c_str(), "%" SCNu64, &imsi);

        bool eutran_avp_found = false;

        if(air.requested_eutran_authentication_info.number_of_requested_vectors.get(num_vectors)){
            eutran_avp_found = true;
            if ( num_vectors > AUTH_MAX_EUTRAN_VECTORS ) {
                result_code = ER_DIAMETER_INVALID_AVP_VALUE;
                break;
            }
        }

        if(air.requested_eutran_authentication_info.re_synchronization_info.get(auts, auts_len)){
            eutran_avp_found = true;
            auts_set = true;
        }
        if(!eutran_avp_found) {

            if(air.requested_utran_geran_authentication_info.number_of_requested_vectors.get(num_vectors)){
                result_code = DIAMETER_ERROR_RAT_NOT_ALLOWED;
                experimental = true;
                break;
            }
            if(air.requested_utran_geran_authentication_info.re_synchronization_info.get(auts, auts_len)){
                result_code = DIAMETER_ERROR_RAT_NOT_ALLOWED;
                experimental = true;
                break;
            }
        }

        if(air.visited_plmn_id.get(plmn_id, plmn_len)){
            if(plmn_len == 3 ){
                if (! Options::getroamallow()) {
                  if (apply_access_restriction ((char*)imsi_str.c_str(), plmn_id) != 0) {
                    result_code = DIAMETER_ERROR_ROAMING_NOT_ALLOWED;
                    experimental = true;
                    break;
                  }
                }
            }
            else{
                result_code = ER_DIAMETER_INVALID_AVP_VALUE;
                break;
            }
        }
        else{
            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
            break;
        }

        if (! m_app.dataaccess().getImsiSec (imsi_str, imsisec) ) {
          result_code = DIAMETER_AUTHENTICATION_DATA_UNAVAILABLE;
          experimental = true;
          break;
        }

        if (auts_set) {
            sqn = sqn_ms_derive_cpp (imsisec.opc, imsisec.key, auts, imsisec.rand);
            if (sqn != NULL) {

              //We succeeded to verify SQN_MS...
              //Pick a new RAND and store SQN_MS + RAND in the HSS
              generate_random_cpp (vector[0].rand, RAND_LENGTH);

              m_app.dataaccess().updateRandSqn(imsi_str, vector[0].rand, sqn);
              m_app.dataaccess().incSqn( imsi_str, sqn );

              free (sqn);
            }

             //Fetch new user data
            if (! m_app.dataaccess().getImsiSec (imsi_str, imsisec) ) {
              result_code = DIAMETER_AUTHENTICATION_DATA_UNAVAILABLE;
              experimental = true;
              break;
            }
        }

        sqn = imsisec.sqn;
        for (int i = 0; i < num_vectors; i++) {
            generate_random_cpp (vector[i].rand, RAND_LENGTH);
            generate_vector_cpp (imsisec.opc, imsi, imsisec.key, plmn_id, sqn, &vector[i]);
        }
        m_app.dataaccess().updateRandSqn (imsi_str, vector[num_vectors-1].rand, sqn);
        m_app.dataaccess().incSqn( imsi_str, sqn );

        for (int i = 0; i < num_vectors; i++) {
            FDAvp authentication_info ( m_app.getDict().avpAuthenticationInfo() );
            FDAvp eurtran_vector ( m_app.getDict().avpEUtranVector() );

            eurtran_vector.add(m_app.getDict().avpRand(),  vector[i].rand,  sizeof(vector[i].rand) );
            eurtran_vector.add(m_app.getDict().avpXres(),  vector[i].xres,  sizeof(vector[i].xres) );
            eurtran_vector.add(m_app.getDict().avpAutn(),  vector[i].autn,  sizeof(vector[i].autn) );
            eurtran_vector.add(m_app.getDict().avpKasme(), vector[i].kasme, sizeof(vector[i].kasme));

            authentication_info.add(eurtran_vector);
            ans.add(authentication_info);
        }

    }while(false);

    //Handle errors
    if (DIAMETER_ERROR_IS_VENDOR (result_code) && experimental) {
        FDAvp experimental_result ( m_app.getDict().avpExperimentalResult() );
        experimental_result.add( m_app.getDict().avpVendorId(),  VENDOR_3GPP);
        experimental_result.add( m_app.getDict().avpExperimentalResultCode(),  result_code);
        ans.add(experimental_result);
    }
    else{
        ans.add( m_app.getDict().avpResultCode(), result_code);
    }


ans.dump();
    ans.send();

    return 0;
}
 
// INSDR Request (req) Command member functions

IDRRreq::IDRRreq(Application &app, FDMessageRequest *cir_req,
                 EvenStatusMap *evt_map, RIRBuilder *rirbuilder, std::string &imsi, std::string &msisdn):
      INSDRreq       ( app ),
      cir_req        (cir_req),
      evt_map        (evt_map),
      m_rirbuilder   (rirbuilder),
      m_imsi         (imsi),
      m_msisdn       (msisdn)
{
}

// A handler for Answers corresponding to this specific Request
void IDRRreq::processAnswer( FDMessageAnswer &ans )
{
ans.dump();
   //Extract the IDA.
   InsertSubscriberDataAnswerExtractor ida(ans, getApplication().getDict() );

   //Build the CIA from the stored request.
   s6as6d::Application * s6aApp = fdHss.gets6as6dApp();
   s6t::Application * s6tApp = fdHss.gets6tApp();

   FDMessageAnswer cia ( cir_req );

   cia.add( s6tApp->getDict().avpAuthSessionState(), 1 );
   cia.addOrigin();
   cia.add( s6tApp->getDict().avpResultCode(), ER_DIAMETER_SUCCESS);

   uint32_t ida_result_code = 0;
   //check the global status from the IDA response
   ida.result_code.get(ida_result_code);

   if(!m_rirbuilder){

      //////////////////////////////////
      /////Single IMSI case
      //////////////////////////////////

      //Fill the long term event result status based on the insertion result on db
      for (EvenStatusMap::iterator it= evt_map->begin(); it!=evt_map->end(); ++it){
         if(it->second.isLongTermEvt()){
            FDAvp mon_evt_cfg_status( s6tApp->getDict().avpMonitoringEventConfigStatus() );
            fdHss.buildCfgStatusAvp(mon_evt_cfg_status, it->second);
            cia.add(mon_evt_cfg_status);
         }
      }

      if(ida_result_code == DIAMETER_SUCCESS){

         //////////////////////////////////////////////////////////
         ///// Response OK from MME
         ///// Build a CIA response including
         ///// + For the long term events: The config status of
         /////   reflecting the DB operation (failure/ok)
         ///// + For the one-shot events: The response from
         /////   the mme
         //////////////////////////////////////////////////////////

         //Fill the one-shot event result based on the response from the mme.
         for( std::list<MonitoringEventConfigStatusExtractor*>::iterator it = ida.monitoring_event_config_status.getList().begin();
               it != ida.monitoring_event_config_status.getList().end(); ++ it)
         {
            std::string    scef_id;
            uint32_t       scef_ref_id;
            (*it)->scef_id.get(scef_id);
            (*it)->scef_reference_id.get(scef_ref_id);

            bool service_report_found = false;
            std::list<ServiceReportExtractor*>::iterator it_service;
            if( !(*it)->service_report.getList().empty() ){
               service_report_found = true;
               it_service = (*it)->service_report.getList().begin();
            }

            EvenStatusMap::iterator map_iter = evt_map->find( std::make_pair(scef_id, scef_ref_id) );

            if(map_iter != evt_map->end()){

               if( !map_iter->second.isLongTermEvt() ){
                  uint32_t  service_result_code;
                  if(service_report_found && (*it_service)->service_result.service_result_code.get(service_result_code)){
                     std::cout << "service_result found on the mme response, updating to " << service_result_code << std::endl;
                     map_iter->second.result = service_result_code;
                  }
                  //We only add the one shot events
                  FDAvp mon_evt_cfg_status( s6tApp->getDict().avpMonitoringEventConfigStatus() );
                  fdHss.buildCfgStatusAvp(mon_evt_cfg_status, map_iter->second);
                  cia.add(mon_evt_cfg_status);
               }
            }
         }
      }
      else{

         //////////////////////////////////////////////////////////
         ///// Response ok from MME
         ///// Build a CIA response including
         ///// + For the long term events: The config status of
         /////   reflecting the DB operation (failure/ok)
         ///// + For the one-shot events: status DIAMETER_ERROR_USER_UNKNOWN
         ///// + Include a cia flag indicating ABSENT_SUBSCRIBER
         //////////////////////////////////////////////////////////

         for (EvenStatusMap::iterator it= evt_map->begin(); it!=evt_map->end(); ++it){
            if(!it->second.isLongTermEvt()){
               it->second.result = DIAMETER_ERROR_USER_UNKNOWN;
               FDAvp mon_evt_cfg_status( s6tApp->getDict().avpMonitoringEventConfigStatus() );
               fdHss.buildCfgStatusAvp(mon_evt_cfg_status, it->second);
               cia.add(mon_evt_cfg_status);
            }
         }
         //Indicate error on mme by setting the S6T-HSS-Cause absent subscriber
         cia.add(s6tApp->getDict().avpS6tHssCause(), ABSENT_SUBSCRIBER);
      }

cia.dump();

      delete evt_map;
      cia.send();
      if(cir_req != NULL){
         delete cir_req;
         cir_req = NULL;
      }
   }
   else{

      //////////////////////////////////
      /////Group IMSI case
      //////////////////////////////////


      if(ida_result_code == DIAMETER_SUCCESS){

         //////////////////////////////////////////////////////////
         ///// Response OK from MME
         ///// Store mme response for further inclusion on RIR
         //////////////////////////////////////////////////////////

         EvenStatusMap* mme_response = new EvenStatusMap();

         for( std::list<MonitoringEventConfigStatusExtractor*>::iterator it = ida.monitoring_event_config_status.getList().begin();
               it != ida.monitoring_event_config_status.getList().end(); ++it)
         {
            std::string    scef_id;
            uint32_t       scef_ref_id;
            (*it)->scef_id.get(scef_id);
            (*it)->scef_reference_id.get(scef_ref_id);

            bool service_report_found = false;
            std::list<ServiceReportExtractor*>::iterator it_service;
            if( !(*it)->service_report.getList().empty() ){
               service_report_found = true;
               it_service = (*it)->service_report.getList().begin();
            }

            MonitoringConfEventStatus mme_status(false);
            mme_status.scef_id = scef_id;
            mme_status.scef_ref_id = scef_ref_id;

            if(service_report_found){
               uint32_t  service_result_code;

               if(service_report_found && (*it_service)->service_result.service_result_code.get(service_result_code)){
                  mme_status.result = service_result_code;
               }
            }
            mme_response->insert( std::make_pair(std::make_pair(scef_id, scef_ref_id), mme_status) );
         }
         HandleMmeResponseEvtMsg *e = new HandleMmeResponseEvtMsg( mme_response, m_imsi, 0 , m_msisdn);
         m_rirbuilder->postMessage( e );
      }
      else{

         //////////////////////////////////////////////////////////
         ///// Response OK from MME
         ///// Store a fake response for further inclusion on RIR
         //////////////////////////////////////////////////////////

         HandleMmeResponseEvtMsg *e = new HandleMmeResponseEvtMsg( NULL, m_imsi, MME_DOWN,  m_msisdn);
         m_rirbuilder->postMessage( e );
      }
   }
}


// Sends a INSDR Request to the corresponding Peer
int Application::sendINSDRreq(s6t::MonitoringEventConfigurationExtractorList &cir_monevtcfg,
                               std::string& imsi, FDMessageRequest *cir_req, EvenStatusMap *evt_map,
                               RIRBuilder * rir_builder)
{

    DAImsiInfo imsi_info;
    //1.get the subscription data from database
    m_dbobj.getImsiInfo((char*)imsi.c_str(), imsi_info);
    bool imsi_attached = (imsi_info.ms_ps_status == "ATTACHED");
    FDPeer peer;
    peer.setDiameterId( (DiamId_t)imsi_info.mmehost.c_str() );
    //peer.setPort(38680);

    std::cout << "peer_state " << peer.getState() << std::endl;

    bool mme_reachable = ( peer.getState() == PSOpen);

    if(!imsi_attached || !mme_reachable){

       if(rir_builder == NULL){
          //simple imsi
          if(!imsi_attached){
             printf("****************Application::sendINSDRreq::IMSI_NOT_ACTIVE\n\n");
             return IMSI_NOT_ACTIVE;
          }
          printf("****************Application::sendINSDRreq::MME_DOWN: %s\n\n", imsi_info.mmehost.c_str());
          return MME_DOWN;
       }
       else{

          printf("****************Application::sendINSDRreq::MULTI IMSI\n\n");

          //multi imsi
          if(!imsi_attached){
             printf("****************Application::sendINSDRreq::POSTING fake IMSI_NOT_ACTIVE\n\n");
             HandleMmeResponseEvtMsg *e = new HandleMmeResponseEvtMsg( NULL, imsi, IMSI_NOT_ACTIVE, imsi_info.str_msisdn);
             rir_builder->postMessage( e );
          }
          else if(!mme_reachable){
             printf("****************Application::sendINSDRreq::POSTING fake MME_DOWN\n\n");
             HandleMmeResponseEvtMsg *e = new HandleMmeResponseEvtMsg( NULL, imsi, MME_DOWN,  imsi_info.str_msisdn);
             rir_builder->postMessage( e );
          }
          return true;
       }
    }


    INSDRreq *s = createINSDRreq(cir_monevtcfg, imsi, cir_req, evt_map, imsi_info, rir_builder);

    try
    {
         if ( s )
         {
s->dump();
              s->send();
         }
    }
    catch ( FDException &ex )
    {
         std::cout << SUtility::currentTime() << " - EXCEPTION - " << ex.what() << std::endl;
         delete s;
         s = NULL;
    }

    // DO NOT free the newly created INSDRreq object!!
    // It will be deleted by the framework after the
    // answer is received and processed.
    return s != NULL;
}

// A factory for INSDR reuqests
INSDRreq *Application::createINSDRreq(s6t::MonitoringEventConfigurationExtractorList &cir_monevtcfg,
                                      std::string& imsi, FDMessageRequest *cir_req, EvenStatusMap *evt_map,
                                      DAImsiInfo &imsi_info, RIRBuilder *rir_builder)
{
    //  creates the INSDRreq object
    INSDRreq *s = new IDRRreq( *this, cir_req,  evt_map, rir_builder, imsi, imsi_info.str_msisdn);

    s->add( getDict().avpSessionId(), s->getSessionId() );

    s->add( getDict().avpAuthSessionState(), 1 );

    s->addOrigin();

    s->add(getDict().avpUserName(), imsi);

    {
       s->add( getDict().avpDestinationHost(), imsi_info.mmehost );
       s->add( getDict().avpDestinationRealm(), imsi_info.mmerealm );

       printf("Subscription data: %s \n", imsi_info.subscription_data.c_str());

       //2.add the subscription data to the message
       fdJsonAddAvps( imsi_info.subscription_data.c_str() , s->getMsg(), NULL );
       //3.create a extractor to get to the subscription data avp
       InsertSubscriberDataRequestExtractor idr( *s, getDict() );
       //4. Get the pointer to the subscription data
       FDAvp subscription_data(getDict().avpSubscriptionData(), (struct avp *)idr.subscription_data.getReference());
       subscription_data.add(cir_monevtcfg);
    }
    return s;
}

// A handler for Answers corresponding to this specific Request
void INSDRreq::processAnswer( FDMessageAnswer &ans )
{
// TODO - This code must be implemented IF the application
// receives Answers for this command, i.e. it sends the
// INSDR Command
}

// INSDR Command (cmd) member function

// Function invoked when a INSDR Command is received
int INSDRcmd::process( FDMessageRequest *req )
{
// TODO - This code must be implemented IF the application
// receives the INSDR command.
    return -1;
}
 
// DESDR Request (req) Command member functions


// Sends a DESDR Request to the corresponding Peer
bool Application::sendDESDRreq(FDPeer &peer)
{
    //TODO - This code may be modified based on specific
    //         processing needs to send the DESDR Command
    DESDRreq *s = createDESDRreq( peer );

    try
    {
         if ( s )
         {
              s->send();
         }
    }
    catch ( FDException &ex )
    {
         std::cout << SUtility::currentTime() << " - EXCEPTION - " << ex.what() << std::endl;
         delete s;
         s = NULL;
    }

    // DO NOT free the newly created DESDRreq object!!
    // It will be deleted by the framework after the
    // answer is received and processed.
    return s != NULL;
}

// A factory for DESDR reuqests
DESDRreq *Application::createDESDRreq(FDPeer &peer)
{
    //  creates the DESDRreq object
    DESDRreq *s = new DESDRreq( *this );

    //TODO - Code must be added to correctly
    //         populate the DESDR request object

    // return the newly created request object
    return s;
}

// A handler for Answers corresponding to this specific Request
void DESDRreq::processAnswer( FDMessageAnswer &ans )
{
// TODO - This code must be implemented IF the application
// receives Answers for this command, i.e. it sends the
// DESDR Command
}

// DESDR Command (cmd) member function

// Function invoked when a DESDR Command is received
int DESDRcmd::process( FDMessageRequest *req )
{
// TODO - This code must be implemented IF the application
// receives the DESDR command.
    return -1;
}
 
// PUUR Request (req) Command member functions


// Sends a PUUR Request to the corresponding Peer
bool Application::sendPUURreq(FDPeer &peer)
{
    //TODO - This code may be modified based on specific
    //         processing needs to send the PUUR Command
    PUURreq *s = createPUURreq( peer );

    try
    {
         if ( s )
         {
              s->send();
         }
    }
    catch ( FDException &ex )
    {
         std::cout << SUtility::currentTime() << " - EXCEPTION - " << ex.what() << std::endl;
         delete s;
         s = NULL;
    }

    // DO NOT free the newly created PUURreq object!!
    // It will be deleted by the framework after the
    // answer is received and processed.
    return s != NULL;
}

// A factory for PUUR reuqests
PUURreq *Application::createPUURreq(FDPeer &peer)
{
    //  creates the PUURreq object
    PUURreq *s = new PUURreq( *this );

    //TODO - Code must be added to correctly
    //         populate the PUUR request object

    // return the newly created request object
    return s;
}

// A handler for Answers corresponding to this specific Request
void PUURreq::processAnswer( FDMessageAnswer &ans )
{
// TODO - This code must be implemented IF the application
// receives Answers for this command, i.e. it sends the
// PUUR Command
}

// PUUR Command (cmd) member function

// Function invoked when a PUUR Command is received
int PUURcmd::process( FDMessageRequest *req )
{
    std::string             s;
    std::string             imsi;
    uint32_t                u32;
    DAMmeIdentity           mme_id;
    int                     result_code = ER_DIAMETER_SUCCESS;
    bool                    experimental = false;

    FDMessageAnswer ans( req );
    ans.addOrigin();
    s6as6d::PurgeUeRequestExtractor pur( *req, m_app.getDict() );

    do {

        pur.auth_session_state.get(u32);
        ans.add( m_app.getDict().avpAuthSessionState(), u32 );

        pur.user_name.get(imsi);
        if(imsi.size() > IMSI_LENGTH){
            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
            break;
        }

        if( pur.pur_flags.get(u32) ){
            if (FLAG_IS_SET (u32, PUR_UE_PURGED_IN_SGSN)) {
              result_code = ER_DIAMETER_INVALID_AVP_VALUE;
              break;
            }
        }

        if( !m_app.dataaccess().getMmeIdentityFromImsi(imsi,mme_id) ){
           experimental = true;
           result_code = DIAMETER_ERROR_USER_UNKNOWN;
           break;
        }

        if( !m_app.dataaccess().purgeUE(imsi) ) {
           experimental = true;
           result_code = DIAMETER_ERROR_USER_UNKNOWN;
           break;
        }

        //Get host and realm
        pur.origin_host.get(s);
        if (mme_id.mme_host != s) {
            result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
            experimental = true;
        }

        pur.origin_realm.get(s);
        if (mme_id.mme_realm != s) {
            result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
            experimental = true;
        }
        ans.add( m_app.getDict().avpPuaFlags(), 1 );

    } while (false);


    //Handle errors
    if (DIAMETER_ERROR_IS_VENDOR (result_code) && experimental) {
        FDAvp experimental_result ( m_app.getDict().avpExperimentalResult() );
        experimental_result.add( m_app.getDict().avpVendorId(),  VENDOR_3GPP);
        experimental_result.add( m_app.getDict().avpExperimentalResultCode(),  result_code);
        ans.add(experimental_result);
    }
    else{
        ans.add( m_app.getDict().avpResultCode(), result_code);
    }

ans.dump();
    ans.send();

    return 0;
}
 
// RER Request (req) Command member functions


// Sends a RER Request to the corresponding Peer
bool Application::sendRERreq(FDPeer &peer)
{
    //TODO - This code may be modified based on specific
    //         processing needs to send the RER Command
    RERreq *s = createRERreq( peer );

    try
    {
         if ( s )
         {
              s->send();
         }
    }
    catch ( FDException &ex )
    {
         std::cout << SUtility::currentTime() << " - EXCEPTION - " << ex.what() << std::endl;
         delete s;
         s = NULL;
    }

    // DO NOT free the newly created RERreq object!!
    // It will be deleted by the framework after the
    // answer is received and processed.
    return s != NULL;
}

// A factory for RER reuqests
RERreq *Application::createRERreq(FDPeer &peer)
{
    //  creates the RERreq object
    RERreq *s = new RERreq( *this );

    //TODO - Code must be added to correctly
    //         populate the RER request object

    // return the newly created request object
    return s;
}

// A handler for Answers corresponding to this specific Request
void RERreq::processAnswer( FDMessageAnswer &ans )
{
// TODO - This code must be implemented IF the application
// receives Answers for this command, i.e. it sends the
// RER Command
}

// RER Command (cmd) member function

// Function invoked when a RER Command is received
int RERcmd::process( FDMessageRequest *req )
{
// TODO - This code must be implemented IF the application
// receives the RER command.
    return -1;
}
 

}
