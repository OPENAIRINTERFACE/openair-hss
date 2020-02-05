#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bstrlib.h"

#include "3gpp_23.003.h"
#include "assertions.h"
#include "common_defs.h"
#include "dynamic_memory_check.h"
#include "etsi_ts_135_206_V10.0.0_annex3.h"
#include "gcc_diag.h"
#include "log.h"
#include "secu_defs.h"
#include "securityDef.h"
#include "usim_authenticate.h"

/****************************************************************************
 **                                                                        **
 ** Name:        usim_authenticate()                                       **
 **                                                                        **
 ** Description: Performs mutual authentication of the USIM to the network,**
 **              checking whether authentication token AUTN can be accep-  **
 **              ted. If so, returns an authentication response RES and    **
 **              the ciphering and integrity keys.                         **
 **              In case of synch failure, returns a re-synchronization    **
 **              token AUTS.                                               **
 **                                                                        **
 **              3GPP TS 31.102, section 7.1.1.1                           **
 **                                                                        **
 **              Authentication and key generating function algorithms are **
 **              specified in 3GPP TS 35.206.                              **
 **                                                                        **
 ** Inputs:      usim_k:        LTE K                                      **
 **              rand:          Random challenge number                    **
 **              autn:          Authentication token                       **
 **                             AUTN = (SQN xor AK) || AMF || MAC          **
 **                                         48          16     64 bits     **
 **              Others:        Security key                               **
 **                                                                        **
 ** Outputs:     auts:          Re-synchronization token                   **
 **              res:           Authentication response                    **
 **              ck:            Ciphering key                              **
 **              ik             Integrity key                              **
 **                                                                        **
 **              Return:        RETURNerror, RETURNok                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int usim_authenticate(usim_data_t* const usim_data /* size 16 */,
                      uint8_t* const rand, uint8_t* const autn,
                      uint8_t* const auts, uint8_t* const res,
                      uint8_t* const ck, uint8_t* const ik) {
  OAILOG_FUNC_IN(LOG_SECU);
  int rc = RETURNok;
  int i;

  // Compute the authentication response RES = f2K (RAND) |
  // Compute the cipher key CK = f3K (RAND)               |
  // Compute the integrity key IK = f4K (RAND)            |=> f2345
  // Compute the anonymity key AK = f5K (RAND)            |
  uint8_t ak[USIM_AK_SIZE];
  f2345(usim_data->lte_k, rand, res, ck, ik, ak);
  OAILOG_TRACE(
      LOG_SECU,
      "LTE K = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      usim_data->lte_k[0], usim_data->lte_k[1], usim_data->lte_k[2],
      usim_data->lte_k[3], usim_data->lte_k[4], usim_data->lte_k[5],
      usim_data->lte_k[6], usim_data->lte_k[7], usim_data->lte_k[8],
      usim_data->lte_k[9], usim_data->lte_k[10], usim_data->lte_k[11],
      usim_data->lte_k[12], usim_data->lte_k[13], usim_data->lte_k[14],
      usim_data->lte_k[15]);
  OAILOG_TRACE(
      LOG_SECU,
      "RAND  = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      rand[0], rand[1], rand[2], rand[3], rand[4], rand[5], rand[6], rand[7],
      rand[8], rand[9], rand[10], rand[11], rand[12], rand[13], rand[14],
      rand[15]);
  OAILOG_TRACE(
      LOG_SECU,
      "AUTN  = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      autn[0], autn[1], autn[2], autn[3], autn[4], autn[5], autn[6], autn[7],
      autn[8], autn[9], autn[10], autn[11], autn[12], autn[13], autn[14],
      autn[15]);
  OAILOG_TRACE(LOG_SECU, "RES   = %02X%02X%02X%02X%02X%02X%02X%02X\n", res[0],
               res[1], res[2], res[3], res[4], res[5], res[6], res[7]);
  OAILOG_TRACE(
      LOG_SECU,
      "CK    = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      ck[0], ck[1], ck[2], ck[3], ck[4], ck[5], ck[6], ck[7], ck[8], ck[9],
      ck[10], ck[11], ck[12], ck[13], ck[14], ck[15]);
  OAILOG_TRACE(
      LOG_SECU,
      "IK    = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      ik[0], ik[1], ik[2], ik[3], ik[4], ik[5], ik[6], ik[7], ik[8], ik[9],
      ik[10], ik[11], ik[12], ik[13], ik[14], ik[15]);

  // Retrieve the sequence number SQN = (SQN ⊕ AK) ⊕ AK
  uint8_t sqn[USIM_SQN_SIZE];
  for (i = 0; i < USIM_SQN_SIZE; i++) {
    sqn[i] = autn[i] ^ ak[i];
  }
  OAILOG_TRACE(LOG_SECU, "Computed SQN   = %02X%02X%02X%02X%02X%02X\n", sqn[0],
               sqn[1], sqn[2], sqn[3], sqn[4], sqn[5]);

  // Compute XMAC = f1K (SQN || RAND || AMF)
  uint8_t xmac[USIM_XMAC_SIZE];
  f1(usim_data->lte_k, rand, sqn, &autn[USIM_SQN_SIZE], xmac);
  OAILOG_TRACE(LOG_SECU,
               "Computed XMAC = f1K (SQN || RAND || AMF): "
               "%02X%02X%02X%02X%02X%02X%02X%02X\n",
               xmac[0], xmac[1], xmac[2], xmac[3], xmac[4], xmac[5], xmac[6],
               xmac[7]);

  // Compare the XMAC with the MAC included in AUTN
#define USIM_AMF_SIZE 2

  if (memcmp(xmac, &autn[USIM_SQN_SIZE + USIM_AMF_SIZE], USIM_XMAC_SIZE) != 0) {
    OAILOG_TRACE(LOG_SECU,
                 "Comparing the XMAC with the MAC included in AUTN Failed\n");
    rc = RETURNerror;
  } else {
    OAILOG_TRACE(
        LOG_SECU,
        "Comparing the XMAC with the MAC included in AUTN Succeeded\n");
    // Verify that the received sequence number SQN is in the correct range
    // TODO rc = usim_check_sqn_range(...);
  }

  if (rc != RETURNok) {
    // Synchronisation failure; compute the AUTS parameter
    // Concealed value of the counter SQNms in the USIM:
    // Conc(SQNMS) = SQNMS ⊕ f5*K(RAND)
    f5star(usim_data->lte_k, rand, ak);

    // consider the SQN received as valid
    uint8_t sqn_ms[USIM_SQNMS_SIZE];
    memset(sqn_ms, 0, USIM_SQNMS_SIZE);

    for (i = 0; i < USIM_SQNMS_SIZE; i++) {
      sqn_ms[i] = usim_data->sqn_ms[i];
    }

    uint8_t sqnms[USIM_SQNMS_SIZE];

    for (i = 0; i < USIM_SQNMS_SIZE; i++) {
      sqnms[i] = sqn_ms[i] ^ ak[i];
    }

    OAILOG_TRACE(LOG_SECU, "SQNms %02X%02X%02X%02X%02X%02X\n", sqnms[0],
                 sqnms[1], sqnms[2], sqnms[3], sqnms[4], sqnms[5]);

    // Synchronisation message authentication code:
    // MACS = f1*K(SQNMS || RAND || AMF)
#define USIM_MACS_SIZE USIM_XMAC_SIZE
    uint8_t macs[USIM_MACS_SIZE];
    f1star(usim_data->lte_k, rand, sqn_ms, &rand[USIM_SQN_SIZE], macs);
    OAILOG_TRACE(
        LOG_SECU,
        "MACS = f1*K(SQNMS || RAND || AMF): %02X%02X%02X%02X%02X%02X%02X%02X\n",
        macs[0], macs[1], macs[2], macs[3], macs[4], macs[5], macs[6], macs[7]);

    // Synchronisation authentication token:
    // AUTS = Conc(SQNMS) || MACS
    // AssertFatal((USIM_SQNMS_SIZE+USIM_MACS_SIZE) < blength(auts), "Auts write
    // out of bounds");
    memcpy(&auts[0], sqnms, USIM_SQNMS_SIZE);
    memcpy(&auts[USIM_SQNMS_SIZE], macs, USIM_MACS_SIZE);
    // btrunc(auts, USIM_SQNMS_SIZE + USIM_MACS_SIZE);
    OAILOG_FUNC_RETURN(LOG_SECU, RETURNerror);
  }
  OAILOG_FUNC_RETURN(LOG_SECU, RETURNok);
}

//------------------------------------------------------------------------------
int usim_authenticate_and_generate_sync_failure(
    usim_data_t* const usim_data /* size 16 */, uint8_t* const rand,
    uint8_t* const autn, uint8_t* const auts, uint8_t* const res,
    uint8_t* const ck, uint8_t* const ik) {
  OAILOG_FUNC_IN(LOG_SECU);
  int rc = RETURNok;
  int i;

  // Compute the authentication response RES = f2K (RAND) |
  // Compute the cipher key CK = f3K (RAND)               |
  // Compute the integrity key IK = f4K (RAND)            |=> f2345
  // Compute the anonymity key AK = f5K (RAND)            |
  uint8_t ak[USIM_AK_SIZE];
  f2345(usim_data->lte_k, rand, res, ck, ik, ak);
  OAILOG_TRACE(
      LOG_SECU,
      "LTE K = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      usim_data->lte_k[0], usim_data->lte_k[1], usim_data->lte_k[2],
      usim_data->lte_k[3], usim_data->lte_k[4], usim_data->lte_k[5],
      usim_data->lte_k[6], usim_data->lte_k[7], usim_data->lte_k[8],
      usim_data->lte_k[9], usim_data->lte_k[10], usim_data->lte_k[11],
      usim_data->lte_k[12], usim_data->lte_k[13], usim_data->lte_k[14],
      usim_data->lte_k[15]);
  OAILOG_TRACE(
      LOG_SECU,
      "RAND  = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      rand[0], rand[1], rand[2], rand[3], rand[4], rand[5], rand[6], rand[7],
      rand[8], rand[9], rand[10], rand[11], rand[12], rand[13], rand[14],
      rand[15]);
  OAILOG_TRACE(
      LOG_SECU,
      "AUTN  = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      autn[0], autn[1], autn[2], autn[3], autn[4], autn[5], autn[6], autn[7],
      autn[8], autn[9], autn[10], autn[11], autn[12], autn[13], autn[14],
      autn[15]);
  OAILOG_TRACE(LOG_SECU, "RES   = %02X%02X%02X%02X%02X%02X%02X%02X\n", res[0],
               res[1], res[2], res[3], res[4], res[5], res[6], res[7]);
  OAILOG_TRACE(
      LOG_SECU,
      "CK    = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      ck[0], ck[1], ck[2], ck[3], ck[4], ck[5], ck[6], ck[7], ck[8], ck[9],
      ck[10], ck[11], ck[12], ck[13], ck[14], ck[15]);
  OAILOG_TRACE(
      LOG_SECU,
      "IK    = "
      "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
      ik[0], ik[1], ik[2], ik[3], ik[4], ik[5], ik[6], ik[7], ik[8], ik[9],
      ik[10], ik[11], ik[12], ik[13], ik[14], ik[15]);

  // Retrieve the sequence number SQN = (SQN ⊕ AK) ⊕ AK
  uint8_t sqn[USIM_SQN_SIZE];
  for (i = 0; i < USIM_SQN_SIZE; i++) {
    sqn[i] = autn[i] ^ ak[i];
  }
  OAILOG_TRACE(LOG_SECU, "Computed SQN   = %02X%02X%02X%02X%02X%02X\n", sqn[0],
               sqn[1], sqn[2], sqn[3], sqn[4], sqn[5]);

  // Compute XMAC = f1K (SQN || RAND || AMF)
  uint8_t xmac[USIM_XMAC_SIZE];
  f1(usim_data->lte_k, rand, sqn, &autn[USIM_SQN_SIZE], xmac);
  OAILOG_TRACE(LOG_SECU,
               "Computed XMAC = f1K (SQN || RAND || AMF): "
               "%02X%02X%02X%02X%02X%02X%02X%02X\n",
               xmac[0], xmac[1], xmac[2], xmac[3], xmac[4], xmac[5], xmac[6],
               xmac[7]);

  // Compare the XMAC with the MAC included in AUTN
#define USIM_AMF_SIZE 2

  if (memcmp(xmac, &autn[USIM_SQN_SIZE + USIM_AMF_SIZE], USIM_XMAC_SIZE) != 0) {
    OAILOG_TRACE(LOG_SECU,
                 "Comparing the XMAC with the MAC included in AUTN Failed\n");
    rc = RETURNerror;
  } else {
    OAILOG_TRACE(
        LOG_SECU,
        "Comparing the XMAC with the MAC included in AUTN Succeeded\n");
    // Verify that the received sequence number SQN is in the correct range
    // TODO rc = usim_check_sqn_range(...);
    rc = RETURNerror;
  }

  if (rc != RETURNok) {
    // Synchronisation failure; compute the AUTS parameter
    // Concealed value of the counter SQNms in the USIM:
    // Conc(SQNMS) = SQNMS ⊕ f5*K(RAND)
    f5star(usim_data->lte_k, rand, ak);

    // consider the SQN received as valid
    uint8_t sqn_ms[USIM_SQNMS_SIZE];
    memset(sqn_ms, 0, USIM_SQNMS_SIZE);

    for (i = 0; i < USIM_SQNMS_SIZE; i++) {
      sqn_ms[i] = usim_data->sqn_ms[i];
    }

    uint8_t sqnms[USIM_SQNMS_SIZE];

    for (i = 0; i < USIM_SQNMS_SIZE; i++) {
      sqnms[i] = sqn_ms[i] ^ ak[i];
    }

    OAILOG_TRACE(LOG_SECU, "SQNms %02X%02X%02X%02X%02X%02X\n", sqnms[0],
                 sqnms[1], sqnms[2], sqnms[3], sqnms[4], sqnms[5]);

    // Synchronisation message authentication code:
    // MACS = f1*K(SQNMS || RAND || AMF)
#define USIM_MACS_SIZE USIM_XMAC_SIZE
    uint8_t macs[USIM_MACS_SIZE];
    f1star(usim_data->lte_k, rand, sqn_ms, &rand[USIM_SQN_SIZE], macs);
    OAILOG_TRACE(
        LOG_SECU,
        "MACS = f1*K(SQNMS || RAND || AMF): %02X%02X%02X%02X%02X%02X%02X%02X\n",
        macs[0], macs[1], macs[2], macs[3], macs[4], macs[5], macs[6], macs[7]);

    // Synchronisation authentication token:
    // AUTS = Conc(SQNMS) || MACS
    // AssertFatal((USIM_SQNMS_SIZE+USIM_MACS_SIZE) < blength(auts), "Auts write
    // out of bounds");
    memcpy(&auts[0], sqnms, USIM_SQNMS_SIZE);
    memcpy(&auts[USIM_SQNMS_SIZE], macs, USIM_MACS_SIZE);
    // btrunc(auts, USIM_SQNMS_SIZE + USIM_MACS_SIZE);
    OAILOG_FUNC_RETURN(LOG_SECU, RETURNerror);
  }
  OAILOG_FUNC_RETURN(LOG_SECU, RETURNok);
}

//------------------------------------------------------------------------------
int usim_generate_kasme(const uint8_t* const autn, const uint8_t* const ck,
                        const uint8_t* const ik, const plmn_t* const plmn,
                        uint8_t* const kasme) {
  /* Compute the derivation key KEY = CK || IK */
  uint8_t key[USIM_CK_SIZE + USIM_IK_SIZE];
  memcpy(key, ck, USIM_CK_SIZE);
  memcpy(key + USIM_CK_SIZE, ik, USIM_IK_SIZE);

  /* Compute the KDF input_s parameter
   * S = FC(0x10) || SNid(MCC, MNC) || 0x00 0x03 || SQN ⊕ AK || 0x00 0x06
   */
  uint8_t input_s[16];            // less than 16
  uint8_t sn_id[AUTH_SNID_SIZE];  // less than 16
  uint16_t length;
  int offset = 0;
  int size_of_length = sizeof(length);

  // FC
  input_s[offset] = 0x10;
  offset += 1;

  // P0=SN id
  length = AUTH_SNID_SIZE;
  sn_id[0] = (plmn->mcc_digit2 << 4) | plmn->mcc_digit1;
  sn_id[1] = (plmn->mnc_digit3 << 4) | plmn->mcc_digit3;
  sn_id[2] = (plmn->mnc_digit2 << 4) | plmn->mnc_digit1;

  memcpy(input_s + offset, sn_id, length);
  OAILOG_TRACE(LOG_SECU,
               "EMM-PROC  _authentication_kasme P0 MCC,MNC %02X %02X %02X\n",
               input_s[offset], input_s[offset + 1], input_s[offset + 2]);
  offset += length;
  // L0=SN id length
  length = htons(length);
  memcpy(input_s + offset, &length, size_of_length);
  offset += size_of_length;

  // P1=Authentication token
  length = AUTH_SQN_SIZE;
  memcpy(input_s + offset, autn, length);
  offset += length;
  // L1
  length = htons(length);
  memcpy(input_s + offset, &length, size_of_length);
  offset += size_of_length;

  /* TODO !!! Compute the Kasme key */
  // todo_hmac_256(key, input_s, kasme->value);
  kdf(key, USIM_CK_SIZE + USIM_IK_SIZE, /*key_length*/
      input_s, offset, kasme, AUTH_KASME_SIZE);

  OAILOG_STREAM_HEX(OAILOG_LEVEL_TRACE, LOG_SECU,
                    "KASME generated by USIM: ", kasme, AUTH_KASME_SIZE);
  return RETURNok;
}
