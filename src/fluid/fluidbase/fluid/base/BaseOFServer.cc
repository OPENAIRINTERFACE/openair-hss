#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <event2/listener.h>
#include <event2/thread.h>
#include <event2/util.h>

#include <string.h>

#include "fluid/TLS.hh"
#include "fluid/base/BaseOFConnection.hh"
#include "fluid/base/BaseOFServer.hh"
#include "fluid/base/EventLoop.hh"

#include <sstream>
#include <string>

namespace fluid_base {

static bool evthread_use_pthreads_called = false;

class BaseOFServer::LibEventBaseOFServer {
 private:
  friend class BaseOFServer;
  struct evconnlistener *listener;
  static void conn_accept_cb(struct evconnlistener *listener,
                             evutil_socket_t fd, struct sockaddr *address,
                             int socklen, void *arg);
  static void conn_accept_error_cb(struct evconnlistener *listener, void *arg);
};

BaseOFServer::BaseOFServer(const char *address_, const int port,
                           const int nthreads, bool secure) {
  // Prepare libevent for threads
  // This will leave a small, insignificant leak for us.
  // See: http://archives.seul.org/libevent/users/Jul-2011/msg00028.html
  if (!evthread_use_pthreads_called) {
    evthread_use_pthreads();
    evthread_use_pthreads_called = true;
  }

  // Ignore SIGPIPE so it becomes an EPIPE
  signal(SIGPIPE, SIG_IGN);

  m_implementation = new BaseOFServer::LibEventBaseOFServer;
  this->m_implementation->listener = NULL;

  this->nconn = 0;

  this->secure = secure;

#if defined(HAVE_TLS)
  if (this->secure && tls_obj == NULL) {
    fprintf(stderr,
            "To establish secure connections, call libfluid_tls_init first.\n");
  }
#endif

  // Create event loops
  // Threads will be created in BaseOFServer::start
  this->nthreads = nthreads;
  this->eventloops = new EventLoop *[nthreads];
  this->threads = new pthread_t[nthreads];
  memset(this->threads, 0, sizeof(pthread_t) * nthreads);
  for (int i = 0; i < nthreads; i++) {
    this->eventloops[i] = new EventLoop(i);
  }
  // The first event loop will be used for connections, so we move to the
  // next one for the first connection
  eventloop = 0;
  if (nthreads > 1) eventloop = 1;

  size_t address_len = strlen(address_) + 1;
  this->address = new char[address_len];
  memcpy(this->address, address_, address_len);
  snprintf(this->port, 6, "%d", port);
}

BaseOFServer::~BaseOFServer() {
  delete[] threads;

  if (this->m_implementation->listener != NULL) {
    evconnlistener_free(this->m_implementation->listener);
    this->m_implementation->listener = NULL;
  }

  // Delete the event loops
  for (int i = 0; i < nthreads; i++) {
    delete eventloops[i];
  }

  delete[] eventloops;

  delete m_implementation;

  delete[] this->address;
}

bool BaseOFServer::start(bool block) {
  this->blocking = block;

  // Start listening for connections in the first event loop
  if (not listen(eventloops[0])) return false;
  if (this->secure) fprintf(stderr, "Secure ");
  fprintf(stderr, "Server running (%s:%s)\n", this->address, this->port);

  // Start one thread for each event loop
  // If we're blocking, the first event loop will run in the calling thread
  for (int i = this->blocking ? 1 : 0; i < nthreads; i++) {
    pthread_create(&threads[i], NULL, EventLoop::thread_adapter, eventloops[i]);
  }

  // Start the first event loop in the calling thread if we're blocking
  if (this->blocking) {
    eventloops[0]->run();
  }

  return true;
}

void BaseOFServer::stop() {
  // Stop listening for new connections
  if (m_implementation->listener != NULL)
    evconnlistener_disable(m_implementation->listener);

  // Ask all event loops to stop
  for (int i = 0; i < nthreads; i++) {
    eventloops[i]->stop();
  }

  // Wait for all threads to finish (which will happen when the event loops
  // stop running)
  for (int i = this->blocking ? 1 : 0; i < nthreads; i++) {
    pthread_join(threads[i], NULL);
  }
}

bool BaseOFServer::listen(EventLoop *evloop) {
  struct event_base *base = (struct event_base *)evloop->get_base();
  ;

  // Hostname lookup
  struct evutil_addrinfo hints;
  struct evutil_addrinfo *result = NULL, *rp;
  int err;
  evutil_socket_t fd;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;  // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = EVUTIL_AI_PASSIVE | EVUTIL_AI_ADDRCONFIG;

  err = evutil_getaddrinfo(this->address, this->port, &hints, &result);
  if (err != 0) {
    fprintf(stderr, "Error resolving '%s': %s\n", this->address,
            evutil_gai_strerror(err));
    return false;
  }

  int v = 1;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
    if (fd == -1) continue;

    if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;

    close(fd);
  }

  if (rp == NULL) {
    fprintf(stderr, "Could not bind to '%s' (%s)\n", this->address,
            strerror(errno));
    freeaddrinfo(result);
    return false;
  }

  freeaddrinfo(result);

  // Listen
  evutil_make_socket_nonblocking(fd);
  m_implementation->listener =
      evconnlistener_new(base, m_implementation->conn_accept_cb, this,
                         LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, fd);
  if (!m_implementation->listener) {
    perror("Error creating connection listener");
    return false;
  }
  evconnlistener_set_error_cb(m_implementation->listener,
                              m_implementation->conn_accept_error_cb);

  return true;
}

EventLoop *BaseOFServer::choose_eventloop() {
  EventLoop *selected_evloop = eventloops[eventloop];
  eventloop = (++eventloop) % nthreads;
  return selected_evloop;
}

void BaseOFServer::base_connection_callback(
    BaseOFConnection *conn, BaseOFConnection::Event event_type) {
  if (event_type == BaseOFConnection::EVENT_CLOSED) delete conn;
}

void BaseOFServer::free_data(void *data) { BaseOFConnection::free_data(data); }

/* Internal libevent callbacks */
void BaseOFServer::LibEventBaseOFServer::conn_accept_cb(
    struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *address, int socklen, void *arg) {
  BaseOFServer *ofserver = static_cast<BaseOFServer *>(arg);
  int id = ofserver->nconn++;
  BaseOFConnection *c = new BaseOFConnection(
      id, ofserver, ofserver->choose_eventloop(), fd, ofserver->secure);
}

void BaseOFServer::LibEventBaseOFServer::conn_accept_error_cb(
    struct evconnlistener *listener, void *arg) {
  struct event_base *base = evconnlistener_get_base(listener);
  int err = EVUTIL_SOCKET_ERROR();
  fprintf(stderr, "BaseOFServer error (%d :%s).", err,
          evutil_socket_error_to_string(err));
}

}  // namespace fluid_base
