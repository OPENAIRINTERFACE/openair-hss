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

#ifndef __OPTIONS_H
#define __OPTIONS_H

#include <stdint.h>
#include <string>

extern "C" {
   #include "hss_config.h"
}


class Options
{
public:

   static bool parse( int argc, char **argv );
   static bool parseInputOptions( int argc, char **argv );
   static bool parseJson();
   static bool validateOptions();
   
   static const std::string &getjsonConfig()             { return m_jsoncfg; }
   static const std::string &getdiameterconfiguration()  { return m_diameterconfiguration; }
   static const std::string &getcassserver()             { return m_cassserver; }
   static const std::string &getcassuser()               { return m_cassuser; }
   static const std::string &getcasspwd()                { return m_casspwd; }
   static const std::string &getcassdb()                 { return m_cassdb; }
   static bool               getrandvector()             { return m_randvector; }
   static const std::string &getoptkey()                 { return m_optkey; }
   static bool               getreloadkey()              { return m_reloadkey; }
   static bool               getonlyloadkey()            { return m_onlyloadkey; }
   static const int         &getgtwport()                { return m_gtwport; }
   static const std::string &getgtwhost()                { return m_gtwhost; }
   static const int         &getrestport()               { return m_restport; }
   static const std::string &getsynchimsi()              { return m_synchimsi; }
   static const std::string &getsynchauts()              { return m_synchauts; }

   static void fillhssconfig(hss_config_t *hss_config_p);

   //static const std::string &setsynchauts(const std::string &auts) { return m_synchauts = auts; }

private:

   enum OptionsSelected {
     jsoncfg                           = 0x01,
     diameterconfiguration             = 0x02,
     cassserver                        = 0x04,
     cassuser                          = 0x08,
     casspwd                           = 0x10,
     cassdb                            = 0x20,
     randvector                        = 0x40,
     optkey                            = 0x80,
     reloadkey                         = 0x100,
     onlyloadkey                       = 0x200,
     gtwport                           = 0x400,
     gtwhost                           = 0x800,
     restport                          = 0x1000
   };

   static void help();

   static const int JSONFILEBUFFER;
   static int options;

   static std::string m_jsoncfg;
   static std::string m_diameterconfiguration;

   static std::string m_cassserver;
   static std::string m_cassuser;
   static std::string m_casspwd;
   static std::string m_cassdb;
   static bool        m_randvector;
   static std::string m_optkey;
   static bool        m_reloadkey;
   static bool        m_onlyloadkey;
   static int         m_gtwport;
   static std::string m_gtwhost;
   static int         m_restport;
   static std::string m_synchimsi;
   static std::string m_synchauts;
};

#endif // #define __OPTIONS_H
