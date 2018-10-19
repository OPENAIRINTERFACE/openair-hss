/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/* This message asks for task initialization */
MESSAGE_DEF(INITIALIZE_MESSAGE, MESSAGE_PRIORITY_MED)

/* This message asks for task activation */
MESSAGE_DEF(ACTIVATE_MESSAGE,   MESSAGE_PRIORITY_MED)

/* This message asks for task deactivation */
MESSAGE_DEF(DEACTIVATE_MESSAGE, MESSAGE_PRIORITY_MED)

/* This message asks for task termination */
MESSAGE_DEF(TERMINATE_MESSAGE,  MESSAGE_PRIORITY_MAX)

/* Test message used for debug */
MESSAGE_DEF(MESSAGE_TEST,       MESSAGE_PRIORITY_MED)

/* Error message  */
MESSAGE_DEF(ERROR_LOG,          MESSAGE_PRIORITY_MAX)
/* Warning message  */
MESSAGE_DEF(WARNING_LOG,        MESSAGE_PRIORITY_MAX)
/* Notice message  */
MESSAGE_DEF(NOTICE_LOG,         MESSAGE_PRIORITY_MED)
/* Info message  */
MESSAGE_DEF(INFO_LOG,           MESSAGE_PRIORITY_MED)
/* Debug message  */
MESSAGE_DEF(DEBUG_LOG,          MESSAGE_PRIORITY_MED)

/* Generic log message for text */
MESSAGE_DEF(GENERIC_LOG,        MESSAGE_PRIORITY_MED)
