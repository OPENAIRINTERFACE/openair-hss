/** @file */
#ifndef __BASEOFSERVER_HH__
#define __BASEOFSERVER_HH__

#include <pthread.h>
#include <string>

#include "BaseOFConnection.hh"
#include "EventLoop.hh"

#include <stdio.h>

namespace fluid_base {

/**
A BaseOFServer manages the very basic functions of OpenFlow connections, such
as notifying of new messages and network-level events. It is an abstract class
that should be overriden by another class to provide OpenFlow features.
*/
class BaseOFServer : public BaseOFHandler {
 public:
  /**
  Create a BaseOFServer.

  @param address address to bind the server
  @param port TCP port on which the server will listen
  @param nevloops number of event loops to run. Connections will be
                  attributed to event loops running on threads on a
                  round-robin fashion. The first event loop will listen for
                  new connections.
  @param secure whether the connections should use TLS. TLS support must
  compiled into the library and you need to call libfluid_ssl_init before you
  can use this feature.
  */
  BaseOFServer(const char* address, const int port, const int nevloops = 1,
               const bool secure = false);
  virtual ~BaseOFServer();

  /**
  Start the server. It will listen at the port declared in the
  constructor, assigning connections to event loops running in threads and
  optionally blocking the calling thread until BaseOFServer::stop is called.

  @param block block the calling thread while the server is running
  */
  virtual bool start(bool block = false);

  /**
  Stop the server. It will stop listening to new connections and  signal the
  event loops to stop running.

  It will eventually unblock BaseOFServer::start if it is blocking.
  */
  virtual void stop();

  // BaseOFHandler methods
  virtual void base_connection_callback(BaseOFConnection* conn,
                                        BaseOFConnection::Event event_type);
  virtual void base_message_callback(BaseOFConnection* conn, void* data,
                                     size_t len) {
    printf("Calling fake msgcb\n");
  };
  virtual void free_data(void* data);

 private:
  // TODO: hide part of this in LibEventBaseOFServer
  char* address;
  char port[6];

  EventLoop** eventloops;
  EventLoop* main;
  pthread_t* threads;
  bool blocking;
  bool secure;

  int eventloop;
  int nthreads;
  int nconn;

  bool listen(EventLoop* w);
  EventLoop* choose_eventloop();

  class LibEventBaseOFServer;
  friend class LibEventBaseOFServer;
  LibEventBaseOFServer* m_implementation;
};

}  // namespace fluid_base

#endif
