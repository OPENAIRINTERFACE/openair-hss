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

#ifndef __S6AS6D_IMPL_H
#define __S6AS6D_IMPL_H

#include "s6as6d.h"
#include "fdhss.h"

class DataAccess;

namespace s6t    { class MonitoringEventConfigurationExtractorList; }

namespace s6as6d {

// Member functions that customize the individual application
class Application : public ApplicationBase
{
    friend UPLRreq;
    friend CALRreq;
    friend AUIRreq;
    friend INSDRreq;
    friend DESDRreq;
    friend PUURreq;
    friend RERreq;

public:
    Application( DataAccess &dbobj );
    ~Application();

    UPLRcmd &getUPLRcmd() { return m_cmd_uplr; }
    //CALRcmd &getCALRcmd() { return m_cmd_calr; }
    AUIRcmd &getAUIRcmd() { return m_cmd_auir; }
    //INSDRcmd &getINSDRcmd() { return m_cmd_insdr; }
    //DESDRcmd &getDESDRcmd() { return m_cmd_desdr; }
    PUURcmd &getPUURcmd() { return m_cmd_puur; }
    //RERcmd &getRERcmd() { return m_cmd_rer; }

    // Parameters for sendXXXreq, if present below, may be changed
    // based upon processing needs
    bool sendUPLRreq(FDPeer &peer);
    bool sendCALRreq(FDPeer &peer);
    bool sendAUIRreq(FDPeer &peer);
    int  sendINSDRreq(s6t::MonitoringEventConfigurationExtractorList &cir_monevtcfg,
                      std::string& imsi, FDMessageRequest *cir_req, EvenStatusMap *evt_map,
                      RIRBuilder * rir_builder);
    bool sendDESDRreq(FDPeer &peer);
    bool sendPUURreq(FDPeer &peer);
    bool sendRERreq(FDPeer &peer);

    DataAccess &dataaccess() { return m_dbobj; }

private:
    void registerHandlers();
    UPLRcmd m_cmd_uplr;
    //CALRcmd m_cmd_calr;
    AUIRcmd m_cmd_auir;
    //INSDRcmd m_cmd_insdr;
    //DESDRcmd m_cmd_desdr;
    PUURcmd m_cmd_puur;
    //RERcmd m_cmd_rer;

    // the parameters for createXXXreq, if present below, may be
    // changed based processing needs
    UPLRreq *createUPLRreq(FDPeer &peer);
    CALRreq *createCALRreq(FDPeer &peer);
    AUIRreq *createAUIRreq(FDPeer &peer);
    INSDRreq *createINSDRreq(s6t::MonitoringEventConfigurationExtractorList &cir_monevtcfg,
                            std::string& imsi, FDMessageRequest *cir_req, EvenStatusMap *evt_map,
                            DAImsiInfo &imsi_info, RIRBuilder * rir_builder);
    DESDRreq *createDESDRreq(FDPeer &peer);
    PUURreq *createPUURreq(FDPeer &peer);
    RERreq *createRERreq(FDPeer &peer);

    DataAccess &m_dbobj;
};

class IDRRreq : public INSDRreq {

public:
   IDRRreq(Application &app, FDMessageRequest *cir_req,
          EvenStatusMap *evt_map, RIRBuilder *rirbuilder, std::string &imsi, std::string &msisdn);

   void processAnswer( FDMessageAnswer &ans );

private:
   FDMessageRequest *cir_req;
   EvenStatusMap *evt_map;
   RIRBuilder *m_rirbuilder;
   std::string m_imsi;
   std::string m_msisdn;
};

}

#endif // __S6AS6D_IMPL_H
