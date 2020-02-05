/* Copyright (c) 2008 The Board of Trustees of The Leland Stanford
 * Junior University
 *
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */

/* OpenFlow: protocol between controller and datapath.

This is a simplified version of the OpenFlow header that should be valid for
all OpenFlow versions.
*/

#ifndef OPENFLOW_OPENFLOW_H
#define OPENFLOW_OPENFLOW_H 1

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#ifdef SWIG
#define OFP_ASSERT(EXPR) /* SWIG can't handle OFP_ASSERT. */
#elif !defined(__cplusplus)
/* Build-time assertion for use in a declaration context. */
#define OFP_ASSERT(EXPR)                  \
  extern int(*build_assert(void))[sizeof( \
      struct { unsigned int build_assert_failed : (EXPR) ? 1 : -1; })]
#else /* __cplusplus */
#define OFP_ASSERT(_EXPR) typedef int build_assert_failed[(_EXPR) ? 1 : -1]
#endif /* __cplusplus */

#ifndef SWIG
#define OFP_PACKED __attribute__((packed))
#else
#define OFP_PACKED /* SWIG doesn't understand __attribute. */
#endif

enum ofp_type {
  /* Immutable messages. */
  OFPT_HELLO,        /* Symmetric message */
  OFPT_ERROR,        /* Symmetric message */
  OFPT_ECHO_REQUEST, /* Symmetric message */
  OFPT_ECHO_REPLY,   /* Symmetric message */
  OFPT_VENDOR,       /* Symmetric message */

  /* Switch configuration messages. */
  OFPT_FEATURES_REQUEST, /* Controller/switch message */
  OFPT_FEATURES_REPLY,   /* Controller/switch message */
};

/* Header on all OpenFlow packets. */
struct ofp_header {
  uint8_t version; /* OFP_VERSION. */
  uint8_t type;    /* One of the OFPT_ constants. */
  uint16_t length; /* Length including this ofp_header. */
  uint32_t xid;    /* Transaction id associated with this packet.
                      Replies use the same id as was in the request
                      to facilitate pairing. */
};
OFP_ASSERT(sizeof(struct ofp_header) == 8);

/* Hello elements types. */
enum ofp_hello_elem_type {
  OFPHET_VERSIONBITMAP = 1, /* Bitmap of version supported. */
};

/* Common header for all Hello Elements */
struct ofp_hello_elem_header {
  uint16_t type;   /* One of OFPHET_*. */
  uint16_t length; /* Length in bytes of this element. */
};
OFP_ASSERT(sizeof(struct ofp_hello_elem_header) == 4);

/* Version bitmap Hello Element */
struct ofp_hello_elem_versionbitmap {
  uint16_t type;   /* OFPHET_VERSIONBITMAP. */
  uint16_t length; /* Length in bytes of this element. */
  /* Followed by:
   *   - Exactly (length - 4) bytes containing the bitmaps, then
   *   - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
   *     bytes of all-zero bytes */
  uint32_t bitmaps[0]; /* List of bitmaps - supported versions */
};
OFP_ASSERT(sizeof(struct ofp_hello_elem_versionbitmap) == 4);

/* OFPT_HELLO. This message includes zero or more hello elements having
 * variable size. Unknown elements types must be ignored/skipped, to allow
 * for future extensions. */
struct ofp_hello {
  struct ofp_header header;                 /* Hello element list */
  struct ofp_hello_elem_header elements[0]; /* List of elements - 0 or more */
};
OFP_ASSERT(sizeof(struct ofp_hello) == 8);

/* Values for 'type' in ofp_error_message.  These values are immutable: they
 * will not change in future versions of the protocol (although new values may
 * be added). */
enum ofp_error_type {
  OFPET_HELLO_FAILED, /* Hello protocol failed. */
};

/* ofp_error_msg 'code' values for OFPET_HELLO_FAILED.  'data' contains an
 * ASCII text string that may give failure details. */
enum ofp_hello_failed_code {
  OFPHFC_INCOMPATIBLE, /* No compatible version. */
};

/* OFPT_ERROR: Error message (datapath -> controller). */
struct ofp_error_msg {
  struct ofp_header header;

  uint16_t type;
  uint16_t code;
  uint8_t data[0]; /* Variable-length data.  Interpreted based
                      on the type and code. */
};
OFP_ASSERT(sizeof(struct ofp_error_msg) == 12);

/* OFPT_FEATURES_REPLY: Reply to features request message (switch ->
 * controller). */
struct ofp_switch_features {
  struct ofp_header header;
  uint64_t datapath_id;
  uint32_t n_buffers;
  uint8_t n_tables;
  uint8_t auxiliary_id;
  uint8_t pad[2];
  uint32_t capabilities;
  uint32_t reserved;
};
OFP_ASSERT(sizeof(struct ofp_switch_features) == 32);

#endif
