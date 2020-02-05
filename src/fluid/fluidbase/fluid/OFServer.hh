/** @file */
#ifndef __OFSERVER_HH__
#define __OFSERVER_HH__

#include <pthread.h>

#include <map>

#include "OFConnection.hh"
#include "OFServerSettings.hh"
#include "base/BaseOFConnection.hh"
#include "base/BaseOFServer.hh"

/**
Classes for creating an OpenFlow server that listens to connections and handles
events.
*/
namespace fluid_base {
/**
An OFServer manages OpenFlow connections and abstracts their events through
callbacks. It provides some of the basic functionalities: OpenFlow connection
setup and liveness check.

Tipically a controller or low-level controller base class will inherit from
OFServer and implement the message_callback and connection_callback methods
to implement further functionality.
*/
class OFServer : private BaseOFServer, public OFHandler {
 public:
  /**
  Create an OFServer.

  @param address address to bind the server
  @param port TCP port on which the server will listen
  @param nthreads number of threads to run. Connections will be attributed to
                   event loops running on threads on a round-robin fashion.
                   The first event loop will also listen for new connections.
  @param secure whether the connections should use TLS. TLS support must
         compiled into the library and you need to call libfluid_ssl_init
         before you can use this feature.
  @param ofsc the optional server configuration parameters. If no value is
         provided, default settings will be used. See OFServerSettings.
  */
  OFServer(const char* address, const int port, const int nthreads = 4,
           const bool secure = false,
           const struct OFServerSettings ofsc = OFServerSettings());
  virtual ~OFServer();

  /**
  Start the server. It will listen at the port declared in the
  constructor, handling connections in different threads and optionally
  blocking the calling thread until OFServer::stop is called.

  @param block block the calling thread while the server is running
  */
  // We reimplement this so that SWIG bindings don't have know BaseOFServer
  virtual bool start(bool block = false);

  /**
  Stop the server. It will close all connections, ask the theads handling
  connections to finish.

  It will eventually unblock OFServer::start if it is blocking.
  */
  virtual void stop();

  /**
  Retrieve an OFConnection object associated with this OFServer with a given
  id.

  @param id OFConnection id
  */
  OFConnection* get_ofconnection(int id);

  /**
  Set configuration parameters for this OFServer.

  This method should be called before OFServer::start is called. Doing
  otherwise will result in undefined settings behavior. In theory, it will
  work fine, but unpredictable behavior can happen, and some settings will
  only apply to new connections.

  You will usually initialize the settings in the constructor. This method
  is provided to give more flexibility to implementations.

  @param ofsc an OFServerSettings object with the desired settings
  */
  void set_config(OFServerSettings ofsc);

  virtual void connection_callback(OFConnection* conn,
                                   OFConnection::Event event_type){};
  virtual void message_callback(OFConnection* conn, uint8_t type, void* data,
                                size_t len){};
  virtual void free_data(void* data);

 protected:
  OFServerSettings ofsc;
  std::map<int, OFConnection*> ofconnections;
  pthread_mutex_t ofconnections_lock;

  inline void lock_ofconnections() { pthread_mutex_lock(&ofconnections_lock); }

  inline void unlock_ofconnections() {
    pthread_mutex_unlock(&ofconnections_lock);
  }

  void base_message_callback(BaseOFConnection* c, void* data, size_t len);
  void base_connection_callback(BaseOFConnection* c,
                                BaseOFConnection::Event event_type);
  static void* send_echo(void* arg);
};

}  // namespace fluid_base

#endif
