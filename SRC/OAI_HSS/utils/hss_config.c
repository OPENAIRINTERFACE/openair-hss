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


/*! \file hss_config.c
   \brief Base configuration for the HSS. Parse command line and configuration file
   \author Sebastien ROUX <sebastien.roux@eurecom.fr>
   \date 2013
   \version 0.1
*/

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <libconfig.h>

#include "hss_config.h"


#ifndef PACKAGE_NAME
#  define PACKAGE_NAME "OPENAIR-HSS"
#endif
#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
#ifndef PACKAGE_BUGREPORT
#  define PACKAGE_BUGREPORT "openair4G-devel@eurecom.fr"
#endif

#define HSS_CONFIG_STRING_MAIN_SECTION             "HSS"
#define HSS_CONFIG_STRING_MYSQL_SERVER             "MYSQL_server"
#define HSS_CONFIG_STRING_MYSQL_USER               "MYSQL_user"
#define HSS_CONFIG_STRING_MYSQL_PASS               "MYSQL_pass"
#define HSS_CONFIG_STRING_MYSQL_DB                 "MYSQL_db"
#define HSS_CONFIG_STRING_OPERATOR_KEY             "OPERATOR_key"
#define HSS_CONFIG_STRING_RANDOM                   "RANDOM"
#define HSS_CONFIG_STRING_FREEDIAMETER_CONF_FILE   "FD_conf"


// LG TODO fd_g_debug_lvl
int                                     fd_g_debug_lvl = 1;

static int hss_config_parse_command_line (
  int argc,
  char *argv[],
  hss_config_t * hss_config_p);

static int hss_config_parse_file (hss_config_t * hss_config_p);

static void hss_display_banner(void);

static void hss_config_display (hss_config_t * hss_config_p);

static struct option                    long_options[] = {
  {"config", 1, 0, 'c'},
  {"help", 0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0},
};

static const char                       option_string[] = "c:vh";

int
hss_config_init (
  int argc,
  char *argv[],
  hss_config_t * hss_config_p)
{
  int                                     ret = 0;

  if (hss_config_p == NULL) {
    return EINVAL;
  }

  hss_config_p->valid_op = 0;

  if ((ret = hss_config_parse_command_line (argc, argv, hss_config_p)) != 0) {
    return ret;
  }

  hss_display_banner ();

  if ((ret = hss_config_parse_file (hss_config_p)) != 0) {
    /*
     * Parsing of the file failed. -> abort
     */
    abort ();
  }

  if (hss_config_p->random) {
    if (strcasecmp (hss_config_p->random, "false") == 0) {
      hss_config_p->random_bool = 0;
    } else if (strcasecmp (hss_config_p->random, "true") == 0) {
      hss_config_p->random_bool = 1;
    } else {
      fprintf (stderr, "Error in configuration file: random: %s (allowed values {true,false})\n", hss_config_p->random);
      abort ();
    }
  } else {
    hss_config_p->random = "true";
    hss_config_p->random_bool = 1;
    fprintf (stderr, "Default values for random: %s (allowed values {true,false})\n", hss_config_p->random);
  }

  // post processing for op key
  if (hss_config_p->operator_key) {
    if (strlen (hss_config_p->operator_key) == 32) {
      ret = sscanf (hss_config_p->operator_key,
                    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    (unsigned int *)&hss_config_p->operator_key_bin[0], (unsigned int *)&hss_config_p->operator_key_bin[1],
                    (unsigned int *)&hss_config_p->operator_key_bin[2], (unsigned int *)&hss_config_p->operator_key_bin[3],
                    (unsigned int *)&hss_config_p->operator_key_bin[4], (unsigned int *)&hss_config_p->operator_key_bin[5],
                    (unsigned int *)&hss_config_p->operator_key_bin[6], (unsigned int *)&hss_config_p->operator_key_bin[7],
                    (unsigned int *)&hss_config_p->operator_key_bin[8], (unsigned int *)&hss_config_p->operator_key_bin[9],
                    (unsigned int *)&hss_config_p->operator_key_bin[10], (unsigned int *)&hss_config_p->operator_key_bin[11],
                    (unsigned int *)&hss_config_p->operator_key_bin[12], (unsigned int *)&hss_config_p->operator_key_bin[13], (unsigned int *)&hss_config_p->operator_key_bin[14], (unsigned int *)&hss_config_p->operator_key_bin[15]);

      if (ret != 16) {
        fprintf (stderr, "Error in configuration file: operator key: %s\n", hss_config_p->operator_key);
        abort ();
      }

      hss_config_p->valid_op = 1;
    }
  }

  hss_config_display (hss_config_p);
  return 0;
}

static void
hss_display_banner (
  void)
{
  fprintf (stdout, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  fprintf (stdout, "Please report any bug to: %s\n\n", PACKAGE_BUGREPORT);
}

static void
usage (
  void)
{
  hss_display_banner ();
  fprintf (stdout, "Usage: openair_hss [options]\n\n");
  fprintf (stdout, "Available options:\n");
  fprintf (stdout, "\t--help\n\t-h\n");
  fprintf (stdout, "\t\tPrint this help and return\n\n");
  fprintf (stdout, "\t--config=<path>\n\t-c<path>\n");
  fprintf (stdout, "\t\tSet the configuration file for hss\n");
  fprintf (stdout, "\t\tSee template in conf dir\n\n");
  fprintf (stdout, "\t--version\n\t-v\n");
  fprintf (stdout, "\t\tPrint %s version and return\n", PACKAGE_NAME);
}

static void
hss_config_display (
  hss_config_t * hss_config_p)
{
  fprintf (stdout, "Configuration\n");
  fprintf (stdout, "* Global:\n");
  fprintf (stdout, "\t- File .............: %s\n", hss_config_p->config);
  fprintf (stdout, "* MYSQL:\n");
  fprintf (stdout, "\t- Server ...........: %s\n", hss_config_p->mysql_server);
  fprintf (stdout, "\t- Database .........: %s\n", hss_config_p->mysql_database);
  fprintf (stdout, "\t- User .............: %s\n", hss_config_p->mysql_user);
  fprintf (stdout, "\t- Password .........: %s\n", (hss_config_p->mysql_password == NULL) ? "None" : "*****");
  fprintf (stdout, "* FreeDiameter:\n");
  fprintf (stdout, "\t- Conf file ........: %s\n", hss_config_p->freediameter_config);
  fprintf (stdout, "* Security:\n");
  fprintf (stdout, "\t- Operator key......: %s\n", hss_config_p->operator_key);
  fprintf (stdout, "\t- Random      ......: %s\n", hss_config_p->random);
}

static int
hss_config_parse_command_line (
  int argc,
  char *argv[],
  hss_config_t * hss_config_p)
{
  int                                     c;
  int                                     option_index = 0;

  while ((c = getopt_long (argc, argv, option_string, long_options, &option_index)) != -1) {
    switch (c) {
    case 'c':{
        hss_config_p->config = strdup (optarg);
      }
      break;

    case 'v':{
        /*
         * We display version and return immediately
         */
        hss_display_banner ();
        exit (0);
      }
      break;

    default:
    case 'h':{
        usage ();
        exit (0);
      }
      break;
    }
  }

  return 0;
}

static int
hss_config_parse_file (
  hss_config_t * hss_config_p)
{
  int                                     ret = -1;
  config_t                                cfg;
  const char                             *astring = NULL;
  config_setting_t                       *setting = NULL;

  if (hss_config_p == NULL) {
    return ret;
  }

  if (hss_config_p->config == NULL) {
    return ret;
  }
  config_init(&cfg);

  printf ("Parsing configuration file: %s\n", hss_config_p->config);
  if (! config_read_file(&cfg, hss_config_p->config)) {
    fprintf (stderr, "Failed to parse HSS configuration file %s!\n", hss_config_p->config);
    config_destroy(&cfg);
    return ret;
  }
  setting = config_lookup(&cfg, HSS_CONFIG_STRING_MAIN_SECTION);
  if (setting != NULL) {

    if (  (config_setting_lookup_string( setting, HSS_CONFIG_STRING_MYSQL_SERVER, (const char **)&astring) )) {
      hss_config_p->mysql_server = strdup(astring);
    } else {
      fprintf (stderr, "Failed to parse HSS configuration file token %s astring %s!\n", HSS_CONFIG_STRING_MYSQL_SERVER, astring);
      return ret;
    }

    if (  (config_setting_lookup_string( setting, HSS_CONFIG_STRING_MYSQL_USER, (const char **)&astring) )) {
      hss_config_p->mysql_user = strdup(astring);
    } else {
      fprintf (stderr, "Failed to parse HSS configuration file token %s!\n", HSS_CONFIG_STRING_MYSQL_USER);
      return ret;
    }

    if (  (config_setting_lookup_string( setting, HSS_CONFIG_STRING_MYSQL_PASS, (const char **)&astring) )) {
      hss_config_p->mysql_password = strdup(astring);
    } else {
      fprintf (stderr, "Failed to parse HSS configuration file token %s!\n", HSS_CONFIG_STRING_MYSQL_PASS);
      return ret;
    }

    if (  (config_setting_lookup_string( setting, HSS_CONFIG_STRING_MYSQL_DB, (const char **)&astring) )) {
      hss_config_p->mysql_database = strdup(astring);
    } else {
      fprintf (stderr, "Failed to parse HSS configuration file token %s!\n", HSS_CONFIG_STRING_MYSQL_DB);
      return ret;
    }

    if (  (config_setting_lookup_string( setting, HSS_CONFIG_STRING_OPERATOR_KEY, (const char **)&astring) )) {
      hss_config_p->operator_key = strdup(astring);
    } else {
      fprintf (stderr, "Failed to parse HSS configuration file token %s!\n", HSS_CONFIG_STRING_OPERATOR_KEY);
      return ret;
    }

    if (  (config_setting_lookup_string( setting, HSS_CONFIG_STRING_RANDOM, (const char **)&astring) )) {
      hss_config_p->random = strdup(astring);
    } else {
      fprintf (stderr, "Failed to parse HSS configuration file token %s!\n", HSS_CONFIG_STRING_RANDOM);
      return ret;
   }

    if (  (config_setting_lookup_string( setting, HSS_CONFIG_STRING_FREEDIAMETER_CONF_FILE, (const char **)&astring) )) {
     hss_config_p->freediameter_config = strdup(astring);
    } else {
      fprintf (stderr, "Failed to parse HSS configuration file token %s!\n", HSS_CONFIG_STRING_FREEDIAMETER_CONF_FILE);
      return ret;
    }
  } else {
    fprintf (stderr, "Failed to parse HSS configuration file main HSS section not found!\n");
    return ret;
  }
  return 0;
}
