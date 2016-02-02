/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */


#ifndef LOG_H_
#define LOG_H_

/* asn1c debug */
extern int asn_debug;
extern int asn1_xer_print;
extern int fd_g_debug_lvl;

// #include "mme_config.h"

//typedef int (*log_specific_init_t)(int log_level);

//int log_init(const mme_config_t *mme_config,
//             log_specific_init_t specific_init);

#include <stdarg.h>
#include <stdint.h>


typedef enum {
  MIN_LOG_ENV = 0,
  LOG_MME_ENV = MIN_LOG_ENV,
  LOG_MME_GW_ENV,
  LOG_SPGW_ENV,
  MAX_LOG_ENV
} log_env_t;

typedef enum {
  MIN_LOG_LEVEL = 0,
  LOG_LEVEL_TRACE = MIN_LOG_ENV,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_NOTICE,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_CRITICAL,
  LOG_LEVEL_ALERT,
  LOG_LEVEL_EMERGENCY,
  MAX_LOG_LEVEL
} log_level_t;

typedef enum {
  MIN_LOG_PROTOS = 0,
  LOG_GTPV1U = MIN_LOG_PROTOS,
  LOG_S1AP_MME,
  LOG_MME_APP,
  LOG_NAS_MME,
  LOG_NAS_EMM_MME,
  LOG_NAS_ESM_MME,
  LOG_SPGW_APP_MME,
  LOG_S11_MME,
  LOG_S11_SGW,
  LOG_S6A_MME,
  LOG_UTIL,
  LOG_CONFIG,
  LOG_MSC,
  MAX_LOG_PROTOS,
} log_proto_t;


#if LOG_OAI
int log_init(const log_env_t envP, const log_level_t default_log_levelP, const int max_threadsP);
void log_start_use(void);
void log_flush_messages(void);
void log_end(void);
void log_message (
      const log_level_t log_levelP,
      const log_proto_t protoP,
      const char *const source_fileP,
      const unsigned int line_numP,
      const uint8_t * const bytesP,
      const unsigned int num_bytes,
      char *format,
      ...);

#define LOG_INIT                                       log_init
#define LOG_START_USE                                  log_start_use
#define LOG_END                                        log_end
#define LOG_EMERGENCY(proto, fORMAT, aRGS...)          do { log_message(LOG_LEVEL_EMERGENCY,proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0)/*!< \brief system is unusable */
#define LOG_ALERT(proto, fORMAT, aRGS...)              do { log_message(LOG_LEVEL_ALERT,    proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief action must be taken immediately */
#define LOG_CRITICAL(proto, fORMAT, aRGS...)           do { log_message(LOG_LEVEL_CRITICAL, proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief critical conditions */
#define LOG_ERROR(proto, fORMAT, aRGS...)              do { log_message(LOG_LEVEL_ERROR,    proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief error conditions */
#define LOG_WARNING(proto, fORMAT, aRGS...)            do { log_message(LOG_LEVEL_WARNING,  proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief warning conditions */
#define LOG_NOTICE(proto, fORMAT, aRGS...)             do { log_message(LOG_LEVEL_NOTICE,   proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief normal but significant condition */
#define LOG_INFO(proto, fORMAT, aRGS...)               do { log_message(LOG_LEVEL_INFO,     proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief informational */
#define LOG_DEBUG(proto, fORMAT, aRGS...)              do { log_message(LOG_LEVEL_DEBUG,    proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief informational */
#define LOG_TRACE(proto, fORMAT, aRGS...)              do { log_message(LOG_LEVEL_TRACE,    proto, __FILE__, __LINE__, fORMAT, ##aRGS); } while(0) /*!< \brief informational */
#else
#define LOG_INIT(a,b,c)                                0
#define LOG_START_USE
#define LOG_END
#define LOG_EMERGENCY(proto, fORMAT, aRGS...)          do { fprintf(stderr, "[EMERGE] "fORMAT, ##args); } while(0)
#define LOG_ALERT(proto, fORMAT, aRGS...)              do { fprintf(stderr, "[ALERT] "fORMAT, ##args); } while(0)
#define LOG_CRITICAL(proto, fORMAT, aRGS...)           do { fprintf(stderr, "[CRITIC] "fORMAT, ##args); } while(0)
#define LOG_ERROR(proto, fORMAT, aRGS...)              do { fprintf(stderr, "[ERROR] "fORMAT, ##args); } while(0)
#define LOG_WARNING(proto, fORMAT, aRGS...)            do { fprintf(stderr, "[WARNIN] "fORMAT, ##args); } while(0)
#define LOG_NOTICE(proto, fORMAT, aRGS...)             do { fprintf(stdout, "[NOTICE] "fORMAT, ##args); } while(0)
#define LOG_INFO(proto, fORMAT, aRGS...)               do { fprintf(stdout, "[INFO] "fORMAT, ##args); } while(0)
#define LOG_DEBUG(proto, fORMAT, aRGS...)              do { fprintf(stdout, "[DEBUG] "fORMAT, ##args); } while(0)
#define LOG_TRACE(proto, fORMAT, aRGS...)              do { fprintf(stdout, "[TRACE] "fORMAT, ##args); } while(0)
#endif

#endif /* LOG_H_ */
