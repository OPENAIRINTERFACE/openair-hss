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

#ifndef FILE_GCC_DIAG_SEEN
#define FILE_GCC_DIAG_SEEN

#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402
# define OAI_GCC_DIAG_STR(s) #s
# define OAI_GCC_DIAG_JOINSTR(x,y) OAI_GCC_DIAG_STR(x ## y)
# define OAI_GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
# define OAI_GCC_DIAG_PRAGMA(x) OAI_GCC_DIAG_DO_PRAGMA(GCC diagnostic x)
# if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
# define OAI_GCC_DIAG_OFF(x) OAI_GCC_DIAG_PRAGMA(push) \
                         OAI_GCC_DIAG_PRAGMA(ignored OAI_GCC_DIAG_JOINSTR(-W,x))
# define OAI_GCC_DIAG_ON(x) OAI_GCC_DIAG_PRAGMA(pop)
# else
# define OAI_GCC_DIAG_OFF(x) OAI_GCC_DIAG_PRAGMA(ignored OAI_GCC_DIAG_JOINSTR(-W,x))
# define OAI_GCC_DIAG_ON(x) OAI_GCC_DIAG_PRAGMA(warning OAI_GCC_DIAG_JOINSTR(-W,x))
# endif
#else
# define OAI_GCC_DIAG_OFF(x)
# define OAI_GCC_DIAG_ON(x)
#endif
#endif /* FILE_GCC_DIAG_SEEN */
