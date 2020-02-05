#include <event2/bufferevent.h>
#include <event2/event.h>
#include <fluid/base/BaseOFConnection.hh>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include <arpa/inet.h>
#include <string.h>

#include <fluid/OFConnection.hh>
#include <fluid/OFServer.hh>
#include <fluid/base/BaseOFConnection.hh>

#include "base/BaseOFClient.hh"

/**
Classes for creating an OpenFlow client that creates connections and handles
events.
*/
namespace fluid_base {
/**
An OFClient manages an OpenFlow outgoing connection and abstract events through
callbacks. It provides some of the basic functionalities: OpenFlow connection
setup and liveness check.

Typically a switch or low-level switch controller base class will inherit from
OFClient and implement the message_callback and connection_callback methods
to implement further functionality.
*/
class OFClient : private BaseOFClient, public OFHandler {
 public:
  /**
  Create an OFClient.

  @param address server address to connect to
  @param port TCP port to connect to
  @param ofsc the optional server configuration parameters. If no value is
         provided, default settings will be used. See OFServerSettings.
  */
  OFClient(int id, std::string address, int port,
           const struct OFServerSettings ofsc = OFServerSettings()
                                                    .supported_version(0x01)
                                                    .supported_version(0x04)
                                                    .is_controller(false));

  virtual ~OFClient();

  /**
  Start the client. It will connect to the address and port declared in the
  constructor, optionally blocking the calling thread until OFClient::stop is
  called.

  @param block block the calling thread while the client is running
  */
  virtual bool start(bool block = false);

  virtual void start_conn();

  virtual void stop_conn();

  /**
  Stop the client. It will close the connection, ask the thead handling
  connections to finish.

  It will eventually unblock OFClient::start if it is blocking.
  */
  virtual void stop();

  // Implement your logic here
  virtual void connection_callback(OFConnection* conn,
                                   OFConnection::Event event_type){};
  virtual void message_callback(OFConnection* conn, uint8_t type, void* data,
                                size_t len){};

  void free_data(void* data);

 protected:
  OFConnection* conn;

 private:
  OFServerSettings ofsc;
  void base_message_callback(BaseOFConnection* c, void* data, size_t len);
  void base_connection_callback(BaseOFConnection* c,
                                BaseOFConnection::Event event_type);
  static void* send_echo(void* arg);
};

}  // namespace fluid_base
