/*******************************************************************************
    OpenAirInterface 
    Copyright(c) 1999 - 2014 Eurecom

    (at your option) any later version.


    but WITHOUT ANY WARRANTY; without even the implied warranty of

    along with OpenAirInterface.The full GNU General Public License is 

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr
  
  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*
 * logs.h
 *
 *  Created on: Nov 22, 2013
 *      Author: Laurent Winckel
 */

#ifndef LOGS_H_
#define LOGS_H_

/* Added definition of the g_info log function to complete the set of log functions from "gmessages.h" */

#include <glib/gmessages.h>

#ifdef G_HAVE_ISO_VARARGS

#define g_info(...)     g_log (G_LOG_DOMAIN,        \
                               G_LOG_LEVEL_INFO,    \
                               __VA_ARGS__)

#else

static void
g_info (const gchar *format,
        ...)
{
  va_list args;
  va_start (args, format);
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format, args);
  va_end (args);
}

#endif

#endif /* LOGS_H_ */
