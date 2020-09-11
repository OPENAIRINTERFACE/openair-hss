/*
 * Copyright (c) 2017 Sprint
 *
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FDJSON_H
#define __FDJSON_H

#include <freeDiameter/freeDiameter-host.h>
#include "freeDiameter/libfdproto.h"
#include "freeDiameter/libfdcore.h"

#ifdef __cplusplus
void fdJsonGetJSON(
    msg_or_avp* ref, std::string& json, void (*errfunc)(const char*));
bool fdJsonGetValueOfMember(
    std::string json, std::string member, std::string& value);
bool fdJsonGetApnValueFromSubData(std::string json, std::string& apn);
extern "C" {
#endif

int fdJsonAddAvps(
    const char* json, msg_or_avp* msg, void (*errfunc)(const char*));
const char* fdJsonGetJSON(msg_or_avp* msg, void (*errfunc)(const char*));

#define FDJSON_SUCCESS 0
#define FDJSON_JSON_PARSING_ERROR 1
#define FDJSON_EXCEPTION 2

#ifdef __cplusplus
};
#endif

#endif /* #define __FDJSON_H */
