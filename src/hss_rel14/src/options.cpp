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
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <freeDiameter/freeDiameter-host.h>

#define RAPIDJSON_NAMESPACE hssrapidjson
#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"

#include "options.h"

const int Options::JSONFILEBUFFER = 1024;

int Options::options;

std::string Options::m_jsoncfg;
std::string Options::m_diameterconfiguration;

std::string Options::m_cassserver;
std::string Options::m_cassuser;
std::string Options::m_casspwd;
std::string Options::m_cassdb;
bool        Options::m_randvector;
bool        Options::m_roamallow;
std::string Options::m_optkey;
bool        Options::m_reloadkey;
bool        Options::m_onlyloadkey;
int         Options::m_gtwport;
std::string Options::m_gtwhost;
int         Options::m_restport;
std::string Options::m_synchimsi;
std::string Options::m_synchauts;

void Options::help()
{
   std::cout << std::endl
             << "Usage:  c3pohss -j path  [OPTIONS]..." << std::endl
             << "  -h, --help                   Print help and exit" << std::endl
             << "  -j, --jsoncfg filename       Read the application configuration from this file." << std::endl
             << "  -f, --fdcfg filename         Read the freeDiameter configuration from this file." << std::endl
             << "                               instead of the default location (" DEFAULT_CONF_PATH "/" FD_DEFAULT_CONF_FILENAME ")." << std::endl
             << "  -s, --casssrv host           Cassandra server." << std::endl
             << "  -u, --cassusr user           Cassandra user." << std::endl
             << "  -p, --casspwd password       Cassandra password." << std::endl
             << "  -d, --cassdb db              Cassandra database name." << std::endl
             << "  -r, --randv  boolean         Random subscriber vector generation" << std::endl
             << "  -t, --roamallow  boolean     Allow roaming for subscribers" << std::endl
             << "  -o, --optkey  key            Operator key" << std::endl
             << "  -i, --reloadkey  boolean     Reload operator keys at init" << std::endl
             << "  -q, --onlyloadkey  boolean   Only load operator keys at init" << std::endl
             << "      --synchimsi  imsi        The IMSI to calculate a new SQN for" << std::endl
             << "      --synchauts  auts        The AUTS value returned by the UE in a SYNCH FAILURE" << std::endl
             ;
}

bool Options::parse( int argc, char **argv ){

   bool ret = true;

   ret = parseInputOptions( argc, argv );

   if(ret && !m_jsoncfg.empty()){
      ret &= parseJson();
   }

   ret &= validateOptions();

   return ret;
}

bool Options::parseJson(){

   FILE* fp = fopen(m_jsoncfg.c_str(), "r");

   if(fp == NULL){
      std::cout << "The json config file specified does not exists" << std::endl;
      return false;
   }

   char readBuffer[JSONFILEBUFFER];
   RAPIDJSON_NAMESPACE::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
   RAPIDJSON_NAMESPACE::Document doc;
   doc.ParseStream(is);
   fclose(fp);

   if(!doc.IsObject()) {
      std::cout << "Error parsing the json config file" << std::endl;
      return false;
   }

   if(doc.HasMember("common")){
      const RAPIDJSON_NAMESPACE::Value& commonSection = doc["common"];
      if(!(options & diameterconfiguration) && commonSection.HasMember("fdcfg")){
         if(!commonSection["fdcfg"].IsString()){ std::cout << "Error parsing json value: [fdcfg]" << std::endl; return false; }
         m_diameterconfiguration = commonSection["fdcfg"].GetString();
         options |= diameterconfiguration;
      }
   }
   if(doc.HasMember("hss")){
      const RAPIDJSON_NAMESPACE::Value& hssSection = doc["hss"];
      if(!(options & cassserver) && hssSection.HasMember("casssrv")){
         if(!hssSection["casssrv"].IsString()) { std::cout << "Error parsing json value: [cassserver]" << std::endl; return false; }
         m_cassserver = hssSection["casssrv"].GetString();
         options |= cassserver;
      }
      if(!(options & cassuser) && hssSection.HasMember("cassusr")){
         if(!hssSection["cassusr"].IsString()) { std::cout << "Error parsing json value: [cassuser]" << std::endl; return false; }
         m_cassuser = hssSection["cassusr"].GetString();
         options |= cassuser;
      }
      if(!(options & casspwd) && hssSection.HasMember("casspwd")){
         if(!hssSection["casspwd"].IsString()) { std::cout << "Error parsing json value: [casspwd]" << std::endl; return false; }
         m_casspwd = hssSection["casspwd"].GetString();
         options |= casspwd;
      }
      if(!(options & cassdb) && hssSection.HasMember("cassdb")){
         if(!hssSection["cassdb"].IsString()) { std::cout << "Error parsing json value: [cassdb]" << std::endl; return false; }
         m_cassdb = hssSection["cassdb"].GetString();
         options |= cassdb;
      }
      if(!(options & randvector) && hssSection.HasMember("randv")){
         if(!hssSection["randv"].IsBool()) { std::cout << "Error parsing json value: [randv]" << std::endl; return false; }
         m_randvector = hssSection["randv"].GetBool();
         options |= randvector;
      }
      if(!(options & roamallow) && hssSection.HasMember("roamallow")){
         if(!hssSection["roamallow"].IsBool()) { std::cout << "Error parsing json value: [roamallow]" << std::endl; return false; }
         m_roamallow = hssSection["roamallow"].GetBool();
         options |= roamallow;
      }
      if(!(options & optkey) && hssSection.HasMember("optkey")){
         if(!hssSection["optkey"].IsString()) { std::cout << "Error parsing json value: [optkey]" << std::endl; return false; }
         m_optkey = hssSection["optkey"].GetString();
         options |= optkey;
      }
      if(!(options & reloadkey) && hssSection.HasMember("reloadkey")){
         if(!hssSection["reloadkey"].IsBool()) { std::cout << "Error parsing json value: [reloadkey]" << std::endl; return false; }
         m_reloadkey = hssSection["reloadkey"].GetBool();
         options |= reloadkey;
      }
      if(!(options & gtwport) && hssSection.HasMember("gtwport")){
         if(!hssSection["gtwport"].IsInt()) { std::cout << "Error parsing json value: [gtwport]" << std::endl; return false; }
         m_gtwport = hssSection["gtwport"].GetInt();
         options |= gtwport;
      }
      if(!(options & gtwhost) && hssSection.HasMember("gtwhost")){
         if(!hssSection["gtwhost"].IsString()) { std::cout << "Error parsing json value: [gtwhost]" << std::endl; return false; }
         m_gtwhost = hssSection["gtwhost"].GetString();
         options |= gtwhost;
      }
      if(!(options & restport) && hssSection.HasMember("restport")){
         if(!hssSection["restport"].IsInt()) { std::cout << "Error parsing json value: [restport]" << std::endl; return false; }
         m_restport = hssSection["restport"].GetInt();
         options |= restport;
      }
   }

   return true;
}

bool Options::validateOptions(){

   return (
            (options & diameterconfiguration)
         && (options & cassserver)
         && (options & cassuser)
         && (options & casspwd)
         && (options & cassdb)
         && (options & optkey)
         && (options & gtwport)
         && (options & gtwhost)
         && (options & restport)
         );


}

bool Options::parseInputOptions( int argc, char **argv )
{
   int c;
   int option_index = 0;
   bool result = true;

   struct option long_options[] = {
      { "help",         no_argument,        NULL, 'h' },
      { "jsoncfg",      required_argument,  NULL, 'j' },
      { "fdcfg",        required_argument,  NULL, 'f' },

      { "casssrv",      required_argument,  NULL, 's' },
      { "cassusr",      required_argument,  NULL, 'u' },
      { "casspwd",      required_argument,  NULL, 'p' },
      { "cassdb",       required_argument,  NULL, 'd' },
      { "randv",        no_argument,        NULL, 'r' },
      { "roamallow",    no_argument,        NULL, 't' },
      { "optkey",       required_argument,  NULL, 'o' },
      { "reloadkey",    no_argument,        NULL, 'i' },
      { "onlyloadkey",  no_argument,        NULL, 'q' },
      { "synchimsi",    required_argument,  NULL, 'x' },
      { "synchauts",    required_argument,  NULL, 'y' },
      { NULL,0,NULL,0 }
   };

   // Loop on arguments
   while (1)
   {
      c = getopt_long(argc, argv, "hj:f:s:u:p:d:ro:iqx:y:", long_options, &option_index );
      if (c == -1)
         break;	// Exit from the loop.

      switch (c)
      {
         case 'h': { help(); exit(0);                                                              break; }
         case 'j': { m_jsoncfg = optarg;                       options |= jsoncfg;                 break; }
         case 'f': { m_diameterconfiguration = optarg;         options |= diameterconfiguration;   break; }
         case 's': { m_cassserver = optarg;                    options |= cassserver;              break; }
         case 'u': { m_cassuser = optarg;                      options |= cassuser;                break; }
         case 'p': { m_casspwd = optarg;                       options |= casspwd;                 break; }
         case 'd': { m_cassdb = optarg;                        options |= cassdb;                  break; }
         case 'r': { m_randvector = true;                      options |= randvector;              break; }
         case 't': { m_roamallow = true;                       options |= roamallow;               break; }
         case 'o': { m_optkey = true;                          options |= optkey;                  break; }
         case 'i': { m_reloadkey = true;                       options |= reloadkey;               break; }
         case 'q': { m_onlyloadkey = true;                     options |= onlyloadkey;             break; }
         case 'x': { m_synchimsi = optarg;                                                         break; }
         case 'y': { m_synchauts = optarg;                                                         break; }

         case '?':
         {
            switch ( optopt )
            {
               case 'j': { std::cout << "Option -j (config json file) requires an argument"        << std::endl; break; }
               case 'f': { std::cout << "Option -f (diameter config) requires an argument"         << std::endl; break; }
               case 's': { std::cout << "Option -s (cass server) requires an argument"             << std::endl; break; }
               case 'u': { std::cout << "Option -u (cass user) requires an argument"               << std::endl; break; }
               case 'p': { std::cout << "Option -p (cass pwd) requires an argument"                << std::endl; break; }
               case 'd': { std::cout << "Option -d (cass db) requires an argument"                 << std::endl; break; }
               case 'o': { std::cout << "Option -o (operator key) requires an argument"            << std::endl; break; }
               case 'x': { std::cout << "Option --synchimsi requires an argument"                  << std::endl; break; }
               case 'y': { std::cout << "Option --synchauts requires an argument"                  << std::endl; break; }
               default: { std::cout << "Unrecognized option [" << c << "]"                         << std::endl; break; }
            }
            result = false;
            break;
         }
         default:
         {
            std::cout << "Unrecognized option [" << c << "]" << std::endl;
            result = false;
         }
      }
   }
   return result;
}

void Options::fillhssconfig(hss_config_t *hss_config_p){

   hss_config_p->cassandra_server      = strdup(m_cassserver.c_str());
   hss_config_p->cassandra_user        = strdup(m_cassuser.c_str());
   hss_config_p->cassandra_password    = strdup(m_casspwd.c_str());
   hss_config_p->cassandra_database    = strdup(m_cassdb.c_str());
   hss_config_p->operator_key          = strdup(m_optkey.c_str());
   hss_config_p->freediameter_config   = strdup(m_diameterconfiguration.c_str());


   if (strlen (hss_config_p->operator_key) == 32) {
     int ret = sscanf (hss_config_p->operator_key,
                   "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                   (unsigned int *)&hss_config_p->operator_key_bin[0], (unsigned int *)&hss_config_p->operator_key_bin[1],
                   (unsigned int *)&hss_config_p->operator_key_bin[2], (unsigned int *)&hss_config_p->operator_key_bin[3],
                   (unsigned int *)&hss_config_p->operator_key_bin[4], (unsigned int *)&hss_config_p->operator_key_bin[5],
                   (unsigned int *)&hss_config_p->operator_key_bin[6], (unsigned int *)&hss_config_p->operator_key_bin[7],
                   (unsigned int *)&hss_config_p->operator_key_bin[8], (unsigned int *)&hss_config_p->operator_key_bin[9],
                   (unsigned int *)&hss_config_p->operator_key_bin[10], (unsigned int *)&hss_config_p->operator_key_bin[11],
                   (unsigned int *)&hss_config_p->operator_key_bin[12], (unsigned int *)&hss_config_p->operator_key_bin[13], (unsigned int *)&hss_config_p->operator_key_bin[14], (unsigned int *)&hss_config_p->operator_key_bin[15]);

     if (ret != 16) {
       printf( "Error in configuration file: operator key: %s\n", hss_config_p->operator_key);
       abort ();
     }
     hss_config_p->valid_op = 1;
   }

   if(m_randvector) {
      hss_config_p->random = (char*)"true";
      hss_config_p->random_bool = 1;
   }
   else{
      hss_config_p->random = (char*)"false";
   }
}

