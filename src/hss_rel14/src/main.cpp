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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <iostream>
#include "serror.h"

#include <pistache/endpoint.h>

#include "ssync.h"

#include "s6t_impl.h"
#include "fdhss.h"
#include "options.h"
#include "resthandler.h"

extern "C" {
   #include "hss_config.h"
   #include "aucpp.h"
   #include "auc.h"
}


hss_config_t hss_config;
FDHss fdHss;
static SEvent shutdownEvent;

void handler(int signo, siginfo_t *pinfo, void *pcontext)
{
   std::cout << "caught signal (" << signo << ") initiating shutdown" << std::endl;
   shutdownEvent.set();
}

void initHandler()
{
   struct sigaction sa;
   sa.sa_flags = SA_SIGINFO;
   sa.sa_sigaction = handler;
   sigemptyset(&sa.sa_mask);
   int signo = SIGINT;
   if (sigaction(signo, &sa, NULL) == -1)
      SError::throwRuntimeExceptionWithErrno( "Unable to register SIGINT handler" );
   signo = SIGTERM;
   if (sigaction(signo, &sa, NULL) == -1)
      SError::throwRuntimeExceptionWithErrno( "Unable to register SIGTERM handler" );
}

#include "util.h"

int main(int argc, char **argv)
{
   if ( !Options::parse( argc, argv ) )
   {
      std::cout << "Options::parse() failed" << std::endl;
      return 1;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   std::cout << "Options::jsonConfig            : " << Options::getjsonConfig()            << std::endl;
   std::cout << "Options::diameterconfiguration : " << Options::getdiameterconfiguration() << std::endl;
   std::cout << "Options::cassserver            : " << Options::getcassserver()            << std::endl;
   std::cout << "Options::cassuser              : " << Options::getcassuser()              << std::endl;
   std::cout << "Options::casspwd               : " << Options::getcasspwd()               << std::endl;
   std::cout << "Options::cassdb                : " << Options::getcassdb()                << std::endl;
   std::cout << "Options::randvector            : " << Options::getrandvector()            << std::endl;
   std::cout << "Options::roamallow             : " << Options::getroamallow()             << std::endl;
   std::cout << "Options::optkey                : " << Options::getoptkey()                << std::endl;
   std::cout << "Options::reloadkey             : " << Options::getreloadkey()             << std::endl;
   std::cout << "Options::onlyloadkey           : " << Options::getonlyloadkey()           << std::endl;
   std::cout << "Options::gtwport               : " << Options::getgtwport()               << std::endl;
   std::cout << "Options::gtwhost               : " << Options::getgtwhost()               << std::endl;
   std::cout << "Options::resthost              : " << Options::getrestport()              << std::endl;
   std::cout << "Options::synchimsi             : " << Options::getsynchimsi()             << std::endl;
   std::cout << "Options::synchauts             : " << Options::getsynchauts()             << std::endl;

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   initHandler();

   //Fill the hss_config to be used by c sec
   memset (&hss_config, 0, sizeof (hss_config_t));
   Options::fillhssconfig(&hss_config);

   random_init ();

   fdHss.initdb(&hss_config);

   if( Options::getonlyloadkey() ){
      fdHss.updateOpcKeys( (uint8_t *) hss_config.operator_key_bin );
      return 0;
   }

   if( Options::getreloadkey() ){
      fdHss.updateOpcKeys( (uint8_t *) hss_config.operator_key_bin );
   }

#if 0
   if ( !Options::getsynchimsi().empty() && Options::getsynchauts().empty() )
   {
      DAImsiSec  imsisec;
      uint8_t sqn[6]    = { 0x00, 0x00, 0x00, 0x00, 0x03, 0x80 };
      uint8_t amf[2]    = { 0x00, 0x00 };
      uint8_t mac_s[8];
      uint8_t ak[6];
      uint8_t sqn_ms[6];

      if (!fdHss.getDb().getImsiSec(Options::getsynchimsi(), imsisec))
         return false;

      f1star( imsisec.opc, imsisec.key, imsisec.rand, imsisec.sqn, amf, mac_s );
      f5star( imsisec.opc, imsisec.key, imsisec.rand, ak );

      for (int i=0; i<sizeof(sqn_ms); i++)
         sqn_ms[i] = imsisec.sqn[i] ^ ak[i];

      std::cout << "OPC    : " << Utility::bytes2hex(imsisec.opc, 16, '.') << std::endl;
      std::cout << "KEY    : " << Utility::bytes2hex(imsisec.key, 16, '.') << std::endl;
      std::cout << "RAND   : " << Utility::bytes2hex(imsisec.rand, 16, '.') << std::endl;
      std::cout << "AK     : " << Utility::bytes2hex(ak, 6, '.') << std::endl;
      std::cout << "SQN    : " << Utility::bytes2hex(sqn, 6, '.') << std::endl;
      std::cout << "SQN_MS : " << Utility::bytes2hex(sqn_ms, 6, '.') << std::endl;
      std::cout << "MAC_S  : " << Utility::bytes2hex(mac_s, 8, '.') << std::endl;

      std::string auts;
      auts = Utility::bytes2hex(sqn_ms,6);
      auts.append(Utility::bytes2hex(mac_s,8));
      std::cout << "AUTS   : " << auts << std::endl;
      Options::setsynchauts(auts);
      return 0;
   }
#endif

   if ( !Options::getsynchimsi().empty() && !Options::getsynchauts().empty() ) {
      fdHss.synchFix();
      return 0;
   }

   if ( fdHss.init(&hss_config) )
      shutdownEvent.wait();

   fdHss.shutdown();

   fdHss.waitForShutdown();

   return 0;
}
