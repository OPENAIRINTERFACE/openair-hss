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

#ifndef __SSTATS_H
#define __SSTATS_H

#include <stdlib.h>
#include <string>
#include <iostream>
#include <map>

#include "sthread.h"
#include "ssyslog.h"
#include "stimer.h"
#include "stime.h"



const uint16_t STAT_CONSOLIDATE_EVENT = ETM_USER + 1;


class SStatsSerializer {

public:
   virtual ~SStatsSerializer(){}
   virtual void serialize(std::map<std::string, std::string>& keyValues, std::string& output) = 0;
};

class SStatsSerializerJson : public SStatsSerializer  {

public:

   void serialize(std::map<std::string, std::string>& keyValues, std::string& output);

};


class SStats : public SEventThread
{

public :

   SStats(std::string id);

   virtual ~SStats();

   void onInit();

   void onQuit();

   void onTimer( SEventThread::Timer &t );

   void setInterval(long interval) { m_interval = interval; }

   void dispatch( SEventThreadMessage &msg );

   virtual void getConsolidatedPeriodStat(std::map<std::string, std::string>& keyValues) = 0;

   virtual void addStat(SEventThreadMessage &msg) = 0;

   virtual void resetStats() = 0;


private:

   void addGenerationTimeStamp(std::map<std::string, std::string>& keyValues);

   long m_interval;
   SEventThread::Timer m_idletimer;
   SSysLog m_logger;
   SStatsSerializer  *m_serializer;
   STimerElapsed m_elapsedtimer;

};



#endif /* __SSTATS_H_ */
