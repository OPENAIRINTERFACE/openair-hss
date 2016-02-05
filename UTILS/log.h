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
  LOG_LEVEL_EMERGENCY = MIN_LOG_ENV,
  LOG_LEVEL_ALERT,
  LOG_LEVEL_CRITICAL,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_NOTICE,
  LOG_LEVEL_INFO,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_TRACE,
  MAX_LOG_LEVEL
} log_level_t;

typedef enum {
  MIN_LOG_PROTOS = 0,
  LOG_UDP = MIN_LOG_PROTOS,
  LOG_GTPV1U,
  LOG_GTPV2C,
  LOG_SCTP,
  LOG_S1AP,
  LOG_MME_APP,
  LOG_NAS,
  LOG_NAS_EMM,
  LOG_NAS_ESM,
  LOG_SPGW_APP,
  LOG_S11,
  LOG_S6A,
  LOG_UTIL,
  LOG_CONFIG,
  LOG_MSC,
  LOG_ITTI,
  MAX_LOG_PROTOS,
} log_proto_t;

typedef struct log_queue_item_s {
  char                                   *message_str;
  uint32_t                                message_str_size;
} log_queue_item_t;

typedef struct log_config_s {
  char *output;
  log_level_t   udp_log_level;
  log_level_t   gtpv1u_log_level;
  log_level_t   gtpv2c_log_level;
  log_level_t   sctp_log_level;
  log_level_t   s1ap_log_level;
  log_level_t   nas_log_level;
  log_level_t   mme_app_log_level;
  log_level_t   spgw_app_log_level;
  log_level_t   s11_log_level;
  log_level_t   s6a_log_level;
  log_level_t   util_log_level;
  log_level_t   msc_log_level;
  log_level_t   itti_log_level;
  uint8_t asn1_verbosity_level;
} log_config_t;

#if LOG_OAI
void log_connect_to_server(void);
void log_set_config(const log_config_t * const config);
const char * const log_level_int2str(const log_level_t log_level);
log_level_t log_level_str2int(const char * const log_level_str);
int log_init(
  const log_env_t envP,
  const log_level_t default_log_levelP,
  const int max_threadsP);
void log_itti_connect(void);
void log_start_use(void);
void log_flush_messages(void);
void log_end(void);
void log_stream_hex(
  const log_level_t log_levelP,
  const log_proto_t protoP,
  const char *const source_fileP,
  const unsigned int line_numP,
  const char *const messageP,
  const char *const streamP,
  const unsigned int sizeP);
void log_stream_hex_array(
  const log_level_t log_levelP,
  const log_proto_t protoP,
  const char *const source_fileP,
  const unsigned int line_numP,
  const char *const messageP,
  const char *const streamP,
  const unsigned int sizeP);
void log_message_add (
  log_queue_item_t * contextP,
  char *format,
  ...);
void log_message_finish (
  log_queue_item_t * contextP);
void log_message_start (
  const log_level_t log_levelP,
  const log_proto_t protoP,
  log_queue_item_t ** contextP, // Out parameter
  const char *const source_fileP,
  const unsigned int line_numP,
  char *format,
  ...);
void log_message (
      const log_level_t log_levelP,
      const log_proto_t protoP,
      const char *const source_fileP,
      const unsigned int line_numP,
      char *format,
      ...);
#define LOG_SET_CONFIG                                           log_set_config
#define LOG_LEVEL_STR2INT                                        log_level_str2int
#define LOG_LEVEL_INT2STR                                        log_level_int2str
#define LOG_INIT                                                 log_init
#define LOG_START_USE                                            log_start_use
#define LOG_ITTI_CONNECT                                         log_itti_connect
#define LOG_END()                                                log_end()
#define LOG_EMERGENCY(pRoTo, aRgS...)                            do { log_message(LOG_LEVEL_EMERGENCY,pRoTo, __FILE__, __LINE__, ##aRgS); } while(0)/*!< \brief system is unusable */
#define LOG_ALERT(pRoTo, aRgS...)                                do { log_message(LOG_LEVEL_ALERT,    pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief action must be taken immediately */
#define LOG_CRITICAL(pRoTo, aRgS...)                             do { log_message(LOG_LEVEL_CRITICAL, pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief critical conditions */
#define LOG_ERROR(pRoTo, aRgS...)                                do { log_message(LOG_LEVEL_ERROR,    pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief error conditions */
#define LOG_WARNING(pRoTo, aRgS...)                              do { log_message(LOG_LEVEL_WARNING,  pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief warning conditions */
#define LOG_NOTICE(pRoTo, aRgS...)                               do { log_message(LOG_LEVEL_NOTICE,   pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief normal but significant condition */
#define LOG_INFO(pRoTo, aRgS...)                                 do { log_message(LOG_LEVEL_INFO,     pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief informational */
#define LOG_DEBUG(pRoTo, aRgS...)                                do { log_message(LOG_LEVEL_DEBUG,    pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief debug informations */
#define LOG_TRACE(pRoTo, aRgS...)                                do { log_message(LOG_LEVEL_TRACE,    pRoTo, __FILE__, __LINE__, ##aRgS); } while(0) /*!< \brief most detailled informations, struct dumps */
#define LOG_FUNC_IN(pRoTo)                                       do { log_message(LOG_LEVEL_TRACE,    pRoTo, __FILE__, __LINE__, "Entering %s\n", __FUNCTION__); } while(0) /*!< \brief informational */
#define LOG_FUNC_OUT(pRoTo)                                      do { log_message(LOG_LEVEL_TRACE,    pRoTo, __FILE__, __LINE__, "Leaving %s\n", __FUNCTION__); return;} while(0)
#define LOG_FUNC_RETURN(pRoTo, rEtUrNcOdE)                       do { log_message(LOG_LEVEL_TRACE,    pRoTo, __FILE__, __LINE__, "Leaving %s (rc=%ld)\n", __FUNCTION__, (long)rEtUrNcOdE); return rEtUrNcOdE;} while(0) /*!< \brief informational */
#define LOG_STREAM_HEX(pRoTo, mEsSaGe, sTrEaM, sIzE)             do { log_stream_hex(LOG_LEVEL_TRACE, pRoTo, __FILE__, __LINE__, mEsSaGe, sTrEaM, sIzE); } while(0) /*!< \brief trace buffer content */
#define LOG_STREAM_HEX_ARRAY(pRoTo, mEsSaGe, sTrEaM, sIzE)       do { log_stream_hex_array(LOG_LEVEL_TRACE, pRoTo, __FILE__, __LINE__, mEsSaGe, sTrEaM, sIzE); } while(0) /*!< \brief trace buffer content with indexes */
#define LOG_MESSAGE_START(lOgLeVeL, pRoTo, cOnTeXt, ...)     do { log_message_start(lOgLeVeL, pRoTo, cOnTeXt, __FILE__, __LINE__, ##__VA_ARGS__); } while(0) /*!< \brief when need to log only 1 message with many char messages, ex formating a dumped struct */
#define LOG_MESSAGE_ADD(cOnTeXt, ...)                        do { log_message_add(cOnTeXt, ##__VA_ARGS__); } while(0) /*!< \brief can be called as many times as needed after LOG_MESSAGE_START() */
#define LOG_MESSAGE_FINISH(cOnTeXt)                              do { log_message_finish(cOnTeXt); } while(0) /*!< \brief Send the message built by LOG_MESSAGE_START() n*LOG_MESSAGE_ADD() (n=0..N) */
#else
#define LOG_SET_CONFIG(a)
#define LOG_LEVEL_STR2INT(a)                                     LOG_LEVEL_EMERGENCY
#define LOG_LEVEL_INT2STR(a)                                     "EMERGENCY"
#define LOG_INIT(a,b,c)                                          0
#define LOG_START_USE()
#define LOG_ITTI_CONNECT()
#define LOG_END()
#define LOG_EMERGENCY(pRoTo, aRgS...)                            do { fprintf(stderr, "[EMERGE] "##aRgS); } while(0)
#define LOG_ALERT(pRoTo, aRgS...)                                do { fprintf(stderr, "[ALERT] "##aRgS); } while(0)
#define LOG_CRITICAL(pRoTo, aRgS...)                             do { fprintf(stderr, "[CRITIC] "##aRgS); } while(0)
#define LOG_ERROR(pRoTo, aRgS...)                                do { fprintf(stderr, "[ERROR] " ##aRgS); } while(0)
#define LOG_WARNING(pRoTo, aRgS...)                              do { fprintf(stderr, "[WARNIN] " ##aRgS); } while(0)
#define LOG_NOTICE(pRoTo, aRgS...)                               do { fprintf(stdout, "[NOTICE] "##aRgS); } while(0)
#define LOG_INFO(pRoTo, aRgS...)                                 do { fprintf(stdout, "[INFO] "##aRgS); } while(0)
#define LOG_DEBUG(pRoTo, aRgS...)                                do { fprintf(stdout, "[DEBUG] "##aRgS); } while(0)
#define LOG_TRACE(pRoTo, aRgS...)                                do { fprintf(stdout, "[TRACE] "##aRgS); } while(0)
#define LOG_FUNC_IN(pRoTo)                                       do { fprintf(stdout, "[TRACE] Entering %s\n", __FUNCTION__); } while(0)
#define LOG_FUNC_OUT(pRoTo)                                      do { fprintf(stdout, "[TRACE] Leaving %s\n", __FUNCTION__); } while(0)
#define LOG_FUNC_RETURN(pRoTo, rEtUrNcOdE)                       do { fprintf(stdout, "[TRACE] Leaving %s (rc=%ld)\n", __FUNCTION__, (long)rEtUrNcOdE); return rEtUrNcOdE;} while(0) /*!< \brief informational */
#define LOG_STREAM_HEX_ARRAY(pRoTo, mEsSaGe, sTrEaM, sIzE)
#define LOG_MESSAGE_START(lOgLeVeL, pRoTo, cOnTeXt, aRgS...)
#define LOG_MESSAGE_ADD(cOnTeXt, aRgS...)
#define LOG_MESSAGE_FINISH(cOnTeXt)
#endif

#endif /* LOG_H_ */
