/** @file */
#ifndef __OFSERVERSETTINGS_HH__
#define __OFSERVERSETTINGS_HH__

#include <stdint.h>

namespace fluid_base {

#define ECHO_XID 0x0F
#define HELLO_XID 0x0F

class OFServer;

/**
Configuration parameters for an OFServer. These parameters specify the
OpenFlow behavior of a class that deals with OFConnection objects.
*/
class OFServerSettings {
 public:
  /**
  Create an OFServerSettings with default configuration values.

  Settings will have the following values by default:
  - Only OpenFlow 1.0 is supported (a sane value for the most compatibility)
  - `echo_interval`: `15`
  - `liveness_check`: `true`
  - `handshake`: `true`
  - `dispatch_all_messages`: `false`
  - `use_hello_elems`: `false` (to avoid compatibility issues with
     existing software and hardware)
  - `keep_data_ownership`: `true` (to simplify things)
  - `is_controller`: `true` (OpenFlow controller is normally TCP server, when
     set to false (to indicate OpenFlow switch), the following items must be
     configured to ensure correct values in the features reply message)
  - `datapath_id`: `0`
  - `n_buffers`: `0`
  - `n_tables`: `0`
  - `auxiliary_id`: `0`
  - `capabilities`: `0`

  */
  OFServerSettings();

  /**
  Add a supported version to the set of supported versions.

  Using this method will override the default version (1, OpenFlow 1.0). If
  you call this method with version 4, only version 4 will be supported. If
  you want to add support for version 1, you will need to do so explicitly,
  so that you can choose only the versions you want, while still having a
  nice default.

  @param version OpenFlow protocol version number (e.g.: 4 for OpenFlow 1.3)
  */
  OFServerSettings& supported_version(const uint8_t version);

  /**
  Return an array of OpenFlow versions bitmaps with the supported versions.
  */
  uint32_t* supported_versions();

  /**
  Return the largest version number supported.
  */
  uint8_t max_supported_version();

  /**
  Set the OpenFlow echo interval (in seconds). A connection will be closed if
  no echo replies arrive in this interval, and echo requests will be
  periodically sent using the same interval.

  @param echo_interval the echo interval (in seconds)
  */
  OFServerSettings& echo_interval(const int echo_interval);

  /**
  Return the echo interval.
  */
  int echo_interval();

  /**
  Set whether the OFServer instance should perform liveness checks (timed
  echo requests and replies).

  @param liveness_check true for liveness checking
  */
  OFServerSettings& liveness_check(const bool liveness_check);

  /**
  Return whether liveness check should be performed.
  */
  bool liveness_check();

  /**
  Set whether the OFServer instance should perform OpenFlow handshakes (hello
  messages, version negotiation and features request).

  @param handshake true for automatic OpenFlow handshakes
  */
  OFServerSettings& handshake(const bool handshake);

  /**
  Return whether handshake should be performed.
  */
  bool handshake();

  /**
  Set whether the OFServer instance should forward all OpenFlow messages to
  the user callback (OFHandler::message_callback), including those treated
  for handshake and liveness check.

  @param dispatch_all_messages true to enable forwarding for all messages
  */
  OFServerSettings& dispatch_all_messages(const bool dispatch_all_messages);

  /**
  Return whether all messages should be dispatched.
  */
  bool dispatch_all_messages();

  /**
  Set whether the OFServer instance should send and treat OpenFlow 1.3.1
  hello elements.

  See OFServerSettings::OFServerSettings for more details.

  @param use_hello_elements true to enable hello elems
  */
  OFServerSettings& use_hello_elements(const bool use_hello_elements);

  /**
  Return whether hello elements should be used.
  */
  bool use_hello_elements();

  /**
  Set whether the OFServer instance should own and manage the message data
  passed to its message callback (true) or if your application should be
  responsible for it (false).

  See OFServerSettings::OFServerSettings for more details.

  @param keep_data_ownership true if OFServer is responsible for managing
                             message data, false if your application is.

  */
  OFServerSettings& keep_data_ownership(const bool keep_data_ownership);

  /**
  Return whether message data pointer ownership belongs to OFServer (true) or
  your application (false).
  */
  bool keep_data_ownership();

  /**
  Set whether the OFServer instance represents an OpenFlow controller. This
  affects the initial handshake, a controller will send OFPT_FEATURES_REQUEST
  whereas a switch will respond to such a request with OFPT_FEATURES_REPLY. The
  OpenFlow specification includes this pertinent paragraph about TCP
  server/client role reversal: Optionally, the switch may allow the controller
  to initiate the connection. In this case, the switch should accept incoming
  standard TLS or TCP connections from the controller, using either a
  user-specified transport port or the default OpenFlow transport port 6653 .
  Connections initiated by the switch and the controller behave the same once
  the transport connection is established

  See OFServerSettings::OFServerSettings for more details.

  @param is_controller true for OpenFlow controller, false for OpenFlow switch.
  */
  OFServerSettings& is_controller(const bool is_controller);

  /**
  Return true if instance represents an OpenFlow controller, false for switch.
  */
  bool is_controller();

  /**
  Set the OpenFlow datapath ID. This is relevant for an OpenFlow switch (i.e.
  when is_controller is false), and is needed in the OFPT_FEATURES_REPLY
  message.

  @param datapath_id the datapath ID.
  */
  OFServerSettings& datapath_id(const uint64_t datapath_id);

  /**
  Return the datapath ID.
  */
  uint64_t datapath_id();

  /**
  Set the switch supported n_buffers. This is relevant for an OpenFlow switch
  (i.e. when is_controller is false), and is needed in the OFPT_FEATURES_REPLY
  message.

  @param n_buffers the number of buffers.
  */
  OFServerSettings& n_buffers(const uint32_t n_buffers);

  /**
  Return the number of buffers.
  */
  uint32_t n_buffers();

  /**
  Set the switch supported n_tables. This is relevant for an OpenFlow switch
  (i.e. when is_controller is false), and is needed in the OFPT_FEATURES_REPLY
  message.

  @param n_tables the number of tables.
  */
  OFServerSettings& n_tables(const uint8_t n_tables);

  /**
  Return the number of tables.
  */
  uint8_t n_tables();

  /**
  Set the switch auxiliary ID. This is relevant for an OpenFlow switch (i.e.
  when is_controller is false), and is needed in the OFPT_FEATURES_REPLY
  message.

  @param aux_id the auxiliary ID.
  */
  OFServerSettings& auxiliary_id(const uint8_t aux_id);

  /**
  Return the auxiliary ID.
  */
  uint8_t auxiliary_id();

  /**
  Set the switch capabilities bitmap. This is relevant for an OpenFlow switch
  (i.e. when is_controller is false), and is needed in the OFPT_FEATURES_REPLY
  message.

  @param capabilities the capabilities bitmap.
  */
  OFServerSettings& capabilities(const uint32_t capabilities);

  /**
  Return the capabilities bitmap.
  */
  uint32_t capabilities();

 private:
  friend class OFServer;

  uint32_t _supported_versions;
  uint8_t _max_supported_version;

  bool version_set_by_hand;
  void add_version(const uint8_t version);

  int _echo_interval;
  bool _liveness_check;
  bool _handshake;
  bool _dispatch_all_messages;
  bool _use_hello_elements;
  bool _keep_data_ownership;
  bool _is_controller;
  uint64_t _datapath_id;
  uint32_t _n_buffers;
  uint8_t _n_tables;
  uint8_t _auxiliary_id;
  uint32_t _capabilities;
};

}  // namespace fluid_base

#endif
