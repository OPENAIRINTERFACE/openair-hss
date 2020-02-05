#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>

#include <config.h>
#if defined(HAVE_TLS)
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include "fluid/TLS.hh"
#endif

#include "BaseOFConnection.hh"

namespace fluid_base {

#define OF_HEADER_LENGTH 8

/** An OFReadBuffer holds an OpenFlow message while it is being read and built.

This class is for internal use (it was created to simplify BaseOFConnection),
and it assumes the user will respect the read limits and will always inform
about read data.
*/
class BaseOFConnection::OFReadBuffer {
 public:
  /** Create an BaseOFConnection::OFReadBuffer. */
  OFReadBuffer() { clear(); }
  ~OFReadBuffer() {
    if (data != NULL) delete[] data;
  }

  /** Get how many bytes should be read for this buffer.

  If the buffer is initialized (a complete OpenFlow header has been
  read), it will return how many bytes of the message are still unread.

  If the buffer is unitialized (a complete OpenFlow header has not been
  read), it will return how many bytes of the header still need to be
  read.
  */
  inline uint16_t get_read_len() {
    if (init)
      return this->len - this->pos;
    else
      return OF_HEADER_LENGTH - header_pos;
  }

  /** Get a pointer to the position at which a read operation should put
  the data.
  */
  inline uint8_t* get_read_pos() {
    if (init)
      return this->data + this->pos;
    else
      return this->header + this->header_pos;
  }

  /** Notify the buffer that a read operation was made and a given number
  of bytes is read. This will initialize the buffer for reading a message
  if a complete OpenFlow header was read.

  @param read how many bytes were read
  */
  inline void read_notify(uint16_t read) {
    if (init)
      this->pos += read;
    else {
      this->header_pos += read;
      if (this->header_pos == OF_HEADER_LENGTH) {
        this->len = htons(*((uint16_t*)this->header + 1));
        this->data = new uint8_t[this->len];
        memcpy(this->data, this->header, OF_HEADER_LENGTH);
        this->pos += OF_HEADER_LENGTH;
        init = true;
      }
    }
  }

  /** Check if there is a complete OpenFlow message in the buffer. */
  inline bool is_ready(void) {
    return (this->len != 0) && (this->pos == this->len);
  }

  /** Fetch the data if there is a completely read OpenFlow message in
  the buffer. Return NULL otherwise. */
  inline void* get_data(void) { return this->data; }

  /** Return the length of the message being read in this buffer (in
  bytes). It will return 0 if the OpenFlow header has not been fully
  received yet. */
  inline int get_len() { return this->len; }

  /** Clear the buffer, making it ready to read new messages.

  @param delete_data destroy the dinamically allocated data
                     (false by default)
  */
  inline void clear(bool delete_data = false) {
    if (delete_data and data != NULL) {
      delete[] data;
    }
    this->data = NULL;
    this->init = false;

    memset(this->header, 0, OF_HEADER_LENGTH);
    this->header_pos = 0;

    this->pos = 0;
    this->len = 0;
  }

  /** Free a pointer allocated by this buffer. */
  static void free_data(void* data) { delete[](uint8_t*) data; }

  uint8_t* data;
  bool init;

  uint8_t header[OF_HEADER_LENGTH];
  uint16_t header_pos;

  uint16_t pos;
  uint16_t len;
};

class BaseOFConnection::LibEventBaseOFConnection {
 private:
  friend class BaseOFConnection;

  struct bufferevent* bev;
  struct event* close_event;

  static void event_cb(struct bufferevent* bev, short events, void* arg);
  static void timer_callback(evutil_socket_t fd, short what, void* arg);
  static void immediate_callback(evutil_socket_t fd, short what, void* arg);
  static void read_cb(struct bufferevent* bev, void* arg);
  static void close_cb(int fd, short which, void* arg);
};

BaseOFConnection::BaseOFConnection(int id, BaseOFHandler* ofhandler,
                                   EventLoop* evloop, int fd, bool secure) {
  this->id = id;
  // TODO: move event_base to BaseOFConnection::LibEventBaseOFConnection so
  // we don't need to store this here
  this->evloop = evloop;
  this->buffer = new BaseOFConnection::OFReadBuffer();
  this->manager = NULL;
  this->ofhandler = ofhandler;
  this->m_implementation = new BaseOFConnection::LibEventBaseOFConnection;

  struct event_base* base = (struct event_base*)evloop->get_base();
  this->m_implementation->close_event =
      event_new(base, -1, EV_PERSIST,
                BaseOFConnection::LibEventBaseOFConnection::close_cb, this);
  event_add(this->m_implementation->close_event, NULL);

  this->secure = false;
#if defined(HAVE_TLS)
  if (secure) {
    if (tls_obj != NULL) {
      SSL_CTX* server_ctx = (SSL_CTX*)tls_obj;
      SSL* client_ctx = SSL_new(server_ctx);
      this->m_implementation->bev = bufferevent_openssl_socket_new(
          base, fd, client_ctx, BUFFEREVENT_SSL_ACCEPTING,
          BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
      this->secure = true;
    } else {
      fprintf(stderr,
              "Establishing insecure connection.\nYou must call "
              "libfluid_tls_init first to establish secure connections.\n");
      secure = false;
    }
  }
#endif
  if (not secure) {
    this->m_implementation->bev = bufferevent_socket_new(
        base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
  }

  notify_conn_cb(BaseOFConnection::EVENT_UP);

  bufferevent_setcb(this->m_implementation->bev,
                    BaseOFConnection::LibEventBaseOFConnection::read_cb, NULL,
                    BaseOFConnection::LibEventBaseOFConnection::event_cb, this);
  bufferevent_enable(this->m_implementation->bev, EV_READ | EV_WRITE);
}

BaseOFConnection::~BaseOFConnection() { delete this->m_implementation; }

void BaseOFConnection::send(void* data, size_t len) {
  bufferevent_write(this->m_implementation->bev, data, len);
}

void BaseOFConnection::add_timed_callback(void* (*cb)(void*), int interval,
                                          void* arg) {
  struct timeval tv = {interval / 1000, (interval % 1000) * 1000};
  struct timed_callback* tc = new struct timed_callback;
  tc->cb = cb;
  tc->cb_arg = arg;
  struct event_base* base = (struct event_base*)this->evloop->get_base();
  struct event* ev =
      event_new(base, -1, EV_PERSIST,
                BaseOFConnection::LibEventBaseOFConnection::timer_callback, tc);
  tc->data = ev;
  timed_callbacks.push_back(tc);
  event_add(ev, &tv);
}

void BaseOFConnection::add_immediate_event(void* (*cb)(std::shared_ptr<void>),
                                           std::shared_ptr<void> arg) {
  auto ic = new struct immediate_callback;
  ic->cb = cb;
  ic->cb_arg = arg;
  struct event_base* base = (struct event_base*)this->evloop->get_base();
  // add timeout event with NULL timeval, which adds the event immediately
  // to the event loop
  event_base_once(
      base, -1, EV_TIMEOUT,
      BaseOFConnection::LibEventBaseOFConnection::immediate_callback, ic,
      NULL);  // timeout
}

void BaseOFConnection::set_manager(void* manager) { this->manager = manager; }

void* BaseOFConnection::get_manager() { return this->manager; }

int BaseOFConnection::get_id() { return this->id; }

void BaseOFConnection::close() {
  event_active(this->m_implementation->close_event, EV_READ, 0);
}

void BaseOFConnection::free_data(void* data) {
  BaseOFConnection::OFReadBuffer::free_data(data);
}

/* Private BaseOFConnection methods */
void BaseOFConnection::notify_msg_cb(void* data, size_t n) {
  ofhandler->base_message_callback(this, data, n);
}

void BaseOFConnection::notify_conn_cb(BaseOFConnection::Event event_type) {
  ofhandler->base_connection_callback(this, event_type);
}

void BaseOFConnection::do_close() {
  // Stop all timed callbacks
  struct timed_callback* tc;
  for (std::vector<struct timed_callback*>::iterator it =
           timed_callbacks.begin();
       it != timed_callbacks.end(); it++) {
    tc = *it;
    event_del((struct event*)tc->data);
    event_free((struct event*)tc->data);
    delete tc;
  }

  // Stop the events and delete the buffers
  event_del(this->m_implementation->close_event);
  event_free(this->m_implementation->close_event);

// Workaround for a clean SSL shutdown.
// See:
// http://www.wangafu.net/~nickm/libevent-book/Ref6a_advanced_bufferevents.html
#if defined(HAVE_TLS)
  if (this->secure) {
    SSL* ctx = bufferevent_openssl_get_ssl(this->m_implementation->bev);
    SSL_set_shutdown(ctx, SSL_RECEIVED_SHUTDOWN);
    SSL_shutdown(ctx);
  }
#endif

  bufferevent_free(this->m_implementation->bev);
  delete this->buffer;
  this->buffer = NULL;

  notify_conn_cb(BaseOFConnection::EVENT_CLOSED);
}

/* libevent callbacks */
void BaseOFConnection::LibEventBaseOFConnection::event_cb(
    struct bufferevent* bev, short events, void* arg) {
  BaseOFConnection* c = static_cast<BaseOFConnection*>(arg);

  if (events & BEV_EVENT_ERROR) perror("Connection error");
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
    bufferevent_disable(bev, EV_READ | EV_WRITE);
    c->notify_conn_cb(BaseOFConnection::EVENT_DOWN);
  }
}

void BaseOFConnection::LibEventBaseOFConnection::timer_callback(
    evutil_socket_t fd, short what, void* arg) {
  struct BaseOFConnection::timed_callback* tc =
      static_cast<struct BaseOFConnection::timed_callback*>(arg);
  tc->cb(tc->cb_arg);
}

void BaseOFConnection::LibEventBaseOFConnection::immediate_callback(
    evutil_socket_t fd, short what, void* arg) {
  auto ic = static_cast<struct BaseOFConnection::immediate_callback*>(arg);
  ic->cb(ic->cb_arg);
  delete ic;
}

void BaseOFConnection::LibEventBaseOFConnection::read_cb(
    struct bufferevent* bev, void* arg) {
  BaseOFConnection* c = static_cast<BaseOFConnection*>(arg);

  uint16_t len;
  BaseOFConnection::OFReadBuffer* ofbuf = c->buffer;

  while (1) {
    // Decide how much we should read
    len = ofbuf->get_read_len();
    if (len <= 0) break;

    // Read the data and put it in the buffer
    size_t read = bufferevent_read(bev, ofbuf->get_read_pos(), len);
    if (read <= 0)
      break;
    else
      ofbuf->read_notify(read);

    // Check if the message is fully received and dispatch
    if (ofbuf->is_ready()) {
      void* data = ofbuf->get_data();
      size_t len = ofbuf->get_len();
      ofbuf->clear();
      c->notify_msg_cb(data, len);
    }
  }
}

void BaseOFConnection::LibEventBaseOFConnection::close_cb(int fd, short which,
                                                          void* arg) {
  BaseOFConnection* c = static_cast<BaseOFConnection*>(arg);
  c->do_close();
}

}  // namespace fluid_base
