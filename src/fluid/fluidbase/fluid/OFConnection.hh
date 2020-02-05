/** @file */
#ifndef __OFCONNECTION_HH__
#define __OFCONNECTION_HH__

#include <stdint.h>
#include <stdlib.h>

namespace fluid_base {
class BaseOFConnection;
class OFHandler;

/**
An OFConnection represents an OpenFlow connection with basic protocol
knowledge. It wraps a BaseOFConnection object, providing further abstractions
on top of it.
*/
class OFConnection {
 public:
  /**
  Create an OFConnection.

  @param c the BaseOFConnection object that this OFConnection will represent
  @param ofhandler the OFHandler instance responsible for this OFConnection
  */
  OFConnection(BaseOFConnection* c, OFHandler* ofhandler);

  /** Represents the state of an OFConnection. */
  enum State {
    /** Sent hello message, waiting for reply */
    STATE_HANDSHAKE,
    /** Version negotiation done, features request received */
    STATE_RUNNING,
    /** Version negotiation failed, connection closed */
    STATE_FAILED,
    /** OFConnection is down, unable to send/receive data (it will be closed
    automatically). It represents a disconnection event at the controller
    level. */
    STATE_DOWN
  };

  /**
  OFConnection events. In the descriptions below, "safe to use" means that
  messages can be sent and received normally according to the OpenFlow
  specification.
  */
  enum Event {
    /** The connection has been started, but it is still waiting for the
    OpenFlow handshake. It is not safe to use the connection. */
    EVENT_STARTED,

    /** The connection has been established (OpenFlow handshake complete).
    It is safe to use the connection. */
    EVENT_ESTABLISHED,

    /** The version negotiation has failed because the parts cannot talk in
    a common OpenFlow version. It is not safe to use the connection. */
    EVENT_FAILED_NEGOTIATION,

    /** The connection has been closed. It is not safe to use the
    connection. */
    EVENT_CLOSED,

    /** The connection has been closed due to inactivity (no response to
    echo requests). It is not safe to use the connection. */
    EVENT_DEAD,
  };

  /** Get the connection ID. */
  int get_id();

  /** Check if the connection is alive (responding to echo requests). */
  bool is_alive();

  /** Update the liveness state of the connection. */
  void set_alive(bool alive);

  /**
  Get the connection state. See #OFConnection::State.
  */
  uint8_t get_state();

  /**
  Set the connection state. See #OFConnection::State.

  @param state the new state.
  */
  void set_state(OFConnection::State state);

  /**
  Get the negotiated OpenFlow version for the connection (OpenFlow protocol
  version number). Note that this is not an OFVersion value. It is the value
  that goes into the OpenFlow header (e.g.: 4 for OpenFlow 1.3). */
  uint8_t get_version();

  /**
  Set a negotiated version for the connection. (OpenFlow protocol version
  number). Note that this is not an OFVersion value. It is the value
  that goes into the OpenFlow header (e.g.: 4 for OpenFlow 1.3).

  @param version an OpenFlow version number
  */
  void set_version(uint8_t version);

  /**
  Return the OFHandler instance responsible for the connection.
  */
  OFHandler* get_ofhandler();

  /**
  Send data to through the connection.

  @param data the binary data to send
  @param len length of the binary data (in bytes)
  */
  void send(void* data, size_t len);

  /**
  Set up a function to be called forever with an argument at a regular
  interval. This is a utility function provided for no specific use case, but
  rather because it is frequently needed.

  This method is thread-safe.

  @param cb the callback function. It should accept a void* argument and
            return a void*.
  @param interval interval in milisseconds
  @param arg an argument to the callback function
  */
  void add_timed_callback(void* (*cb)(void*), int interval, void* arg);
  // TODO: add the option for the function to unschedule itself by returning
  // false

  /**
  Set up a function to be called immediately in the event loop. This can be
  used to add events from an external source (i.e. not OVS) in a thread-safe
  manner.

  This method is thread-safe.

  @param cb the callback function. It should accept a shared_ptr  argument and
            return a void*.
  @param arg an argument to the callback function
  */
  void add_immediate_event(void* (*cb)(std::shared_ptr<void>),
                           std::shared_ptr<void> arg);

  /**
  Get application data. This data is any piece of data you might want to
  associated with this OFConnection object.
  */
  void* get_application_data();

  /**
  Set application data.

  See OFConnection::get_application_data.

  @param data a pointer to application data
  */
  void set_application_data(void* data);

  /**
  Close the connection.
  This will not trigger OFServer::connection_callback.
  */
  void close();

 private:
  BaseOFConnection* conn;
  int id;
  State state;
  uint8_t version;
  bool alive;
  OFHandler* ofhandler;
  void* application_data;
};

/**
OFHandler is an abstract class. Its methods must be implemented by classes that
deal with OFConnection events (usually classes that manage one or more
OFConnection objects).
*/
class OFHandler {
 public:
  virtual ~OFHandler() {}

  /**
  Callback for connection events.

  This method blocks the event loop on which the connection is running,
  preventing other connection's events from being handled. If you want to
  perform a very long operation, try to queue it and return.

  The implementation must be thread-safe, because it will possibly be called
  from several threads (on which connection events are being created).

  @param conn the OFConnection on which the message was received
  @param event_type the event type (see #Event)
  */
  virtual void connection_callback(OFConnection* conn,
                                   OFConnection::Event event_type) = 0;

  /**
  Callback for new messages.

  This method blocks the event loop on which the connection is running,
  preventing other connection's events from being handled. If you want to
  perform a very long operation, try to queue it and return.

  The implementation must be thread-safe, because it will possibly be called
  from several threads (on which message events are being created).

  By default, the message data will managed (freed) for you. If you want a
  zero-copy behavior, see OFServerSettings::keep_data_ownership.

  @param conn the OFConnection on which the message was received
  @param type OpenFlow message type
  @param data binary message data
  @param len message length
  */
  virtual void message_callback(OFConnection* conn, uint8_t type, void* data,
                                size_t len) = 0;

  /**
  Free the data passed to OFHandler::message_callback.
  */
  virtual void free_data(void* data) = 0;
};

}  // namespace fluid_base

#endif
