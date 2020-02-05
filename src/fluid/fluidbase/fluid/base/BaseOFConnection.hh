/** @file */
#ifndef __BASEOFCONNECTION_HH__
#define __BASEOFCONNECTION_HH__

#include <stdlib.h>
#include <memory>
#include <vector>

#include "fluid/base/EventLoop.hh"

namespace fluid_base {
class BaseOFHandler;

/**
A BaseOFConnection wraps the basic functionalities of a network connection with
OpenFlow-oriented messaging features. It uses an OFReadBuffer for building the
messages being read and dispatches events to a BaseOFHandler (who created it).

This connection will tipically be wrapped by a higher-level connection object
(a manager object) providing further protocol semantics on top of it.
*/
class BaseOFConnection {
 public:
  /**
  Create a BaseOFConnection.

  @param id connection id
  @param ofhandler the BaseOFHandler for this connection
  @param evloop the EventLoop that will run this connection
  @param fd the OS-level file descriptor for this connection

  @param secure whether the connection should use TLS. TLS support must
  compiled into the library and you need to call libfluid_ssl_init before you
  can use this feature.
  */
  BaseOFConnection(int id, BaseOFHandler* ofhandler, EventLoop* evloop, int fd,
                   bool secure);
  virtual ~BaseOFConnection();

  /** BaseOFConnection events. */
  enum Event {
    /** The connection has been successfully established */
    EVENT_UP,
    /** The other end has ended the connection */
    EVENT_DOWN,
    /** The connection resources have been released and freed */
    EVENT_CLOSED
  };

  /**
  Send a message through this connection.

  This method is thread-safe.

  @param data binary message data
  @param len message length in bytes
  */
  void send(void* data, size_t len);

  /**
  Set up a function to be called forever with an argument at a regular
  interval.

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

  @param cb the callback function. It should accept a shared_ptr argument and
            return a void*.
  @param arg an argument to the callback function
  */
  void add_immediate_event(void* (*cb)(std::shared_ptr<void>),
                           std::shared_ptr<void> arg);

  // TODO: these methods are not thread-safe, and they really aren't
  // currently called from more than one thread. But perhaps we should
  // consider that...

  /**
  Set the manager for this connection. A manager provides  further protocol
  semantics on top of a BaseOFConnection. This manager will tipically be used
  by an upper-level abstraction on top of BaseOFHandler to create its own
  representation of an OpenFlow connection that uses this connection.

  This method is not thread-safe.

  @param manager the manager object
  */
  void set_manager(void* manager);

  /**
  Get the manager for this connection. See BaseOFConnection::set_manager.

  This method is not thread-safe.
  */
  void* get_manager();

  /**
  Get the connection id.
  */
  int get_id();

  /**
  Close this connection. It won't be closed immediately (remaining connection
  and message callbacks may still be called).

  After it is closed, the resources associated with this connection will be
  freed, and no more callbacks will be invoked. Performing any further
  operations on this connection will lead to undefined behavior.

  This method is thread-safe.
  */
  void close();

  /**
  Free the dynamically allocated data sent to the message callback.

  @param data dynamically allocated data sent to the message callback
  */
  static void free_data(void* data);

 private:
  int id;
  EventLoop* evloop;
  class OFReadBuffer;
  OFReadBuffer* buffer;
  void* manager;
  bool secure;
  BaseOFHandler* ofhandler;

  bool running;

  // Types for internal use (timed callbacks)
  struct timed_callback {
    void* (*cb)(void*);
    void* cb_arg;
    void* data;
  };

  struct immediate_callback {
    void* (*cb)(std::shared_ptr<void>);
    std::shared_ptr<void> cb_arg;
  };
  std::vector<struct timed_callback*> timed_callbacks;

  void notify_msg_cb(void* data, size_t n);
  void notify_conn_cb(BaseOFConnection::Event event_type);
  void do_close();

  class LibEventBaseOFConnection;
  friend class LibEventBaseOFConnection;
  LibEventBaseOFConnection* m_implementation;
};

/**
BaseOFHandler is an abstract class. Its methods must be implemented by classes
that deal with BaseOFConnection events (usually classes that manage one or more
BaseOFConnection objects). */
class BaseOFHandler {
 public:
  virtual ~BaseOFHandler() {}

  /**
  Callback for connection events.

  This method blocks the event loop on which the connection is running,
  preventing other connection's events from being handled. If you want to
  perform a very long operation, try to queue it and return.

  The implementation must be thread-safe, because it may be called by several
  EventLoop instances, each running in a thread.

  @param conn the BaseOFConnection on which the message was received
  @param event_type the event type (see #BaseOFConnectionEvent)
  */
  virtual void base_connection_callback(BaseOFConnection* conn,
                                        BaseOFConnection::Event event_type) = 0;

  /**
  Callback for new messages.

  This method blocks the event loop on which the connection is running,
  preventing other connection's events from being handled. If you want to
  perform a very long operation, try to queue it and return.

  The implementation must be thread-safe, because it may be called by several
  EventLoop instances, each running in a thread.

  The message data will not be freed. To free it, you should call
  BaseOFConnection::free_data when you are done with it.

  @param conn the BaseOFConnection on which the message was received
  @param data binary message data
  @param len message length
  */
  virtual void base_message_callback(BaseOFConnection* conn, void* data,
                                     size_t len) = 0;

  /**
  Free the data passed to BaseOFHandler::base_message_callback.
  */
  virtual void free_data(void* data) = 0;
};

}  // namespace fluid_base

#endif
