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

#include <sstream>
#include "sstats.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


SStats::SStats(std::string id)
: m_interval(0),
  m_logger(id)
{

   m_serializer = new SStatsSerializerJson();
}

SStats::~SStats(){

   if(m_serializer != NULL){
      delete m_serializer;
      m_serializer = NULL;
   }
}

void SStats::onInit(){
   m_idletimer.setInterval( m_interval );
   m_idletimer.setOneShot( false );
   initTimer( m_idletimer );
   m_idletimer.start();
   m_elapsedtimer.Start();

}
void SStats::onQuit(){
   stime_t elapsed = m_elapsedtimer.MilliSeconds();
   std::stringstream sstm;
   sstm << elapsed / 1000;
   std::map<std::string, std::string> keyValues;
   keyValues["uptime"]  = sstm.str();
   addGenerationTimeStamp(keyValues);
   std::string serializedStast;
   m_serializer->serialize(keyValues, serializedStast);
   m_logger.syslog(0, "%s", serializedStast.c_str());

}

void SStats::onTimer( SEventThread::Timer &t )
{
   if ( t.getId() == m_idletimer.getId() )
   {
      postMessage( STAT_CONSOLIDATE_EVENT );
   }
}

void SStats::dispatch( SEventThreadMessage &msg )
{
   if ( msg.getId() == STAT_CONSOLIDATE_EVENT )
   {
      std::map<std::string, std::string> keyValues;
      getConsolidatedPeriodStat(keyValues);
      addGenerationTimeStamp(keyValues);
      std::string serializedStast;
      m_serializer->serialize(keyValues, serializedStast);
      m_logger.syslog(0, "%s", serializedStast.c_str());
      resetStats();
   }
   else
   {
      addStat(msg);
   }
}

void SStats::addGenerationTimeStamp(std::map<std::string, std::string>& keyValues){
   STime time_now = STime::Now();
   std::string now_str;
   time_now.Format(now_str, "%Y-%m-%d %H:%M:%S.%0", false);
   keyValues["time_utc"] = now_str;
}

void SStatsSerializerJson::serialize(std::map<std::string, std::string>& keyValues, std::string& output){

   rapidjson::Document document;
   document.SetObject();
   rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

   for (std::map<std::string, std::string>::const_iterator it=keyValues.begin(); it!=keyValues.end(); ++it){

      document.AddMember(rapidjson::StringRef(it->first.c_str()), rapidjson::StringRef(it->second.c_str()), allocator);
   }

   rapidjson::StringBuffer strbuf;
   rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
   document.Accept(writer);
   output =  strbuf.GetString();
}
