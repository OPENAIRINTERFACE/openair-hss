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

/*! \file xml2_wrapper.h
   \brief
   \author  Lionel GAUTHIER
   \date 2016
   \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_XML2_WRAPPER_SEEN
#define FILE_XML2_WRAPPER_SEEN

#define XML_ENCODING "ISO-8859-1"

xmlChar *xml_encoding_convert_input(const char *in, const char *encoding);

//------------------------------------------------------------------------------
// Write an xml comment.
#define XML_WRITE_COMMENT(wRiTeR, cOmMeNt) \
    do { \
      int rc = 0;\
      xmlChar *tmp;\
      tmp = xml_encoding_convert_input(cOmMeNt, XML_ENCODING);\
      rc = xmlTextWriterWriteComment(wRiTeR, tmp);\
      if (rc < 0) {\
        OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterWriteComment\n");\
      }\
      if (tmp != NULL) xmlFree(tmp);\
    } while (0);


//------------------------------------------------------------------------------
// Write an xml comment.
#define XML_WRITE_FORMAT_COMMENT(wRiTeR, ...) \
    do { \
      int rc = 0;\
      rc = xmlTextWriterWriteFormatComment(wRiTeR, ##__VA_ARGS__);\
      if (rc < 0) {\
        OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterWriteFormatComment\n");\
      }\
    } while (0);


//------------------------------------------------------------------------------
// Start an xml element.
#define XML_WRITE_START_ELEMENT(wRiTeR, eLeMeNtNaMe) \
    do { \
      int rc = 0;\
      /* Start an element. */ \
      rc = xmlTextWriterStartElement(wRiTeR, BAD_CAST eLeMeNtNaMe);\
      if (rc < 0) {\
        OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterStartElement\n");\
      }\
    } while (0);


//------------------------------------------------------------------------------
// End the current xml element.
#define XML_WRITE_END_ELEMENT(wRiTeR) \
    do { \
      int rc = 0;\
      /* Close the element */ \
      rc = xmlTextWriterEndElement(wRiTeR);\
      if (rc < 0) {\
        OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterEndElement\n");\
      }\
    } while (0);


//------------------------------------------------------------------------------
// Write an xml attribute.
#define XML_WRITE_ATTRIBUTE(wRiTeR, aTtRiBuTeNaMe, aTtRiBuTeValuE) \
    do { \
      int rc = 0;\
       /* Add an attribute */ \
       rc = xmlTextWriterWriteAttribute(wRiTeR, BAD_CAST aTtRiBuTeNaMe,  BAD_CAST aTtRiBuTeValuE); \
       if (rc < 0) {\
         OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterWriteAttribute\n");\
       }\
    } while (0);


//------------------------------------------------------------------------------
// Write an xml attribute.
#define XML_WRITE_FORMAT_ATTRIBUTE(wRiTeR, aTtRiBuTeNaMe, ...) \
    do { \
      int rc = 0;\
       /* Add an attribute */ \
       rc = xmlTextWriterWriteFormatAttribute(wRiTeR, BAD_CAST aTtRiBuTeNaMe, ##__VA_ARGS__);\
       if (rc < 0) {\
         OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterWriteFormatAttribute\n");\
       }\
    } while (0);


//------------------------------------------------------------------------------
// Write an xml attribute.
#define XML_WRITE_HEX_ATTRIBUTE(wRiTeR, aTtRiBuTeNaMe, bUfFeR, lEnGtH) \
    do { \
      int rc = 0;\
      /* Add an attribute */ \
      if ((bUfFeR) && (lEnGtH)) {\
        char *buffer=malloc((lEnGtH)*2+1);\
        if (buffer) {\
          hexa_to_ascii ((uint8_t *)bUfFeR, buffer,lEnGtH);\
          buffer[2*(lEnGtH)]=0;\
          rc = xmlTextWriterWriteAttribute(wRiTeR, BAD_CAST aTtRiBuTeNaMe,  BAD_CAST buffer); \
          free(buffer);\
          if (rc < 0) {\
            OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterWriteAttribute\n");\
          }\
        } else {\
          OAILOG_ERROR(LOG_XML, "Error at malloc(%d)\n", (lEnGtH)*2+1);\
        }\
      } else {\
        rc = xmlTextWriterWriteAttribute(wRiTeR, BAD_CAST aTtRiBuTeNaMe,  BAD_CAST ""); \
      }\
    } while (0);


//------------------------------------------------------------------------------
// Write a formatted xml element.
#define XML_WRITE_FORMAT_ELEMENT(wRiTeR, eLeMeNtNaMe, ...) \
    do { \
      int rc = 0;\
      /* Write an element */\
      rc = xmlTextWriterWriteFormatElement(wRiTeR, BAD_CAST eLeMeNtNaMe, ##__VA_ARGS__);\
      if (rc < 0) {\
        OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterWriteFormatElement\n");\
      }\
    } while (0);


//------------------------------------------------------------------------------
// Write an xml element.
#define XML_WRITE_HEX_ELEMENT(wRiTeR, eLeMeNtNaMe, bUfFeR, lEnGtH) \
    do { \
      int rc = 0;\
      if ((bUfFeR) && (lEnGtH)) {\
        /* Write an element */\
        char *buffer=malloc((lEnGtH)*2+1);\
        if (buffer) {\
          hexa_to_ascii ((uint8_t *)bUfFeR, buffer,lEnGtH);\
          buffer[2*(lEnGtH)]=0;\
          rc = xmlTextWriterWriteFormatElement(wRiTeR, BAD_CAST eLeMeNtNaMe,  "%s", BAD_CAST buffer); \
          free(buffer);\
          if (rc < 0) {\
            OAILOG_ERROR(LOG_XML, "Error at xmlTextWriterWriteFormatElement\n");\
          }\
        } else {\
          OAILOG_ERROR(LOG_XML, "Error at malloc(%d)\n", (lEnGtH)*2+1);\
        }\
      } else {\
        rc = xmlTextWriterWriteFormatElement(wRiTeR, BAD_CAST eLeMeNtNaMe,  "%s", BAD_CAST ""); \
      }\
    } while (0);

#endif

