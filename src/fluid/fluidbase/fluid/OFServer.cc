#include <arpa/inet.h>
#include <string.h>
#include <algorithm>

#include "fluid/OFConnection.hh"
#include "fluid/OFServer.hh"
#include "fluid/base/BaseOFConnection.hh"
#include "fluid/base/BaseOFServer.hh"
#include "fluid/base/of.hh"

namespace fluid_base {

OFServer::OFServer(const char* address, const int port, const int nthreads,
                   const bool secure, const OFServerSettings ofsc)
    : BaseOFServer(address, port, nthreads, secure) {
  pthread_mutex_init(&ofconnections_lock, NULL);
  this->set_config(ofsc);
}

OFServer::~OFServer() {
  this->lock_ofconnections();
  while (!this->ofconnections.empty()) {
    OFConnection* ofconn = this->ofconnections.begin()->second;
    this->ofconnections.erase(this->ofconnections.begin());
    delete ofconn;
  }
  this->ofconnections.clear();
  this->unlock_ofconnections();
}

bool OFServer::start(bool block) { return BaseOFServer::start(block); }

void OFServer::stop() {
  // Close all connections
  this->lock_ofconnections();
  for (std::map<int, OFConnection*>::iterator it = this->ofconnections.begin();
       it != this->ofconnections.end(); it++) {
    it->second->close();
  }
  this->unlock_ofconnections();

  // Stop BaseOFServer
  BaseOFServer::stop();
}

OFConnection* OFServer::get_ofconnection(int id) {
  this->lock_ofconnections();
  OFConnection* cc = ofconnections[id];
  this->unlock_ofconnections();
  return cc;
}

void OFServer::set_config(OFServerSettings ofsc) { this->ofsc = ofsc; }

void OFServer::base_message_callback(BaseOFConnection* c, void* data,
                                     size_t len) {
  uint8_t version = ((uint8_t*)data)[0];
  uint8_t type = ((uint8_t*)data)[1];
  OFConnection* cc = (OFConnection*)c->get_manager();

  // We trust that the other end is using the negotiated protocol version
  // after the handshake is done. Should we?

  // Should we only answer echo requests after a features reply? The
  // specification isn't clear about that, so we answer whenever an echo
  // request arrives.

  // Handle echo requests
  if (ofsc.liveness_check() and type == OFPT_ECHO_REQUEST) {
    // Just change the type and send back
    ((uint8_t*)data)[1] = OFPT_ECHO_REPLY;
    c->send(data, ntohs(((uint16_t*)data)[1]));

    if (ofsc.dispatch_all_messages())
      goto dispatch;
    else
      goto done;
  }

  // Handle hello messages
  if (ofsc.handshake() and type == OFPT_HELLO) {
    uint32_t client_supported_versions;
    uint32_t overlap;
    uint8_t max_version = this->ofsc.max_supported_version();

    if (ofsc.use_hello_elements() && max_version >= 4 && len > 8 &&
        ntohs(((uint16_t*)data)[4]) == OFPHET_VERSIONBITMAP &&
        ntohs(((uint16_t*)data)[5]) >= 8) {
      // We've both sent and received a version bitmap
      client_supported_versions = ntohl(((uint32_t*)data)[3]);
    } else {
      // Otherwise negotiation picks the minimum supported version
      client_supported_versions = 1 << std::min(version, max_version);
    }

    overlap = *this->ofsc.supported_versions() & client_supported_versions;
    if (overlap) {
      if (ofsc.is_controller()) {
        uint8_t neg_version = 0;
        struct ofp_header msg;
        // Find the highest common support
        while (overlap >>= 1) {
          neg_version++;
        }
        msg.version = neg_version;
        msg.type = OFPT_FEATURES_REQUEST;
        msg.length = htons(8);
        msg.xid = ((uint32_t*)data)[1];
        c->send(&msg, 8);
      }
    } else {
      struct ofp_error_msg msg;
      msg.header.version = version;
      msg.header.type = OFPT_ERROR;
      msg.header.length = htons(12);
      msg.header.xid = ((uint32_t*)data)[1];
      msg.type = htons(OFPET_HELLO_FAILED);
      msg.code = htons(OFPHFC_INCOMPATIBLE);
      cc->send(&msg, 12);

      cc->close();
      cc->set_state(OFConnection::STATE_FAILED);
      connection_callback(cc, OFConnection::EVENT_FAILED_NEGOTIATION);
    }

    if (ofsc.dispatch_all_messages())
      goto dispatch;
    else
      goto done;
  }

  // Handle echo replies (by registering them)
  if (ofsc.liveness_check() and type == OFPT_ECHO_REPLY) {
    if (ntohl(((uint32_t*)data)[1]) == ECHO_XID) {
      cc->set_alive(true);
    }

    if (ofsc.dispatch_all_messages())
      goto dispatch;
    else
      goto done;
  }

  if (ofsc.handshake() and !ofsc.is_controller() and
      type == OFPT_FEATURES_REQUEST) {
    struct ofp_switch_features reply;

    cc->set_version(((uint8_t*)data)[0]);
    cc->set_state(OFConnection::STATE_RUNNING);
    reply.header.version = ((uint8_t*)data)[0];
    reply.header.type = OFPT_FEATURES_REPLY;
    reply.header.length = htons(sizeof(reply));
    reply.header.xid = ((uint32_t*)data)[1];
    reply.datapath_id = ofsc.datapath_id();
    reply.n_buffers = ofsc.n_buffers();
    reply.n_tables = ofsc.n_tables();
    reply.auxiliary_id = ofsc.auxiliary_id();
    reply.capabilities = ofsc.capabilities();
    cc->send(&reply, sizeof(reply));

    if (ofsc.liveness_check())
      c->add_timed_callback(send_echo, ofsc.echo_interval() * 1000, cc);
    connection_callback(cc, OFConnection::EVENT_ESTABLISHED);

    if (ofsc.dispatch_all_messages())
      goto dispatch;
    else
      goto done;
  }

  // Handle feature replies
  if (ofsc.handshake() and ofsc.is_controller() and
      type == OFPT_FEATURES_REPLY) {
    cc->set_version(((uint8_t*)data)[0]);
    cc->set_state(OFConnection::STATE_RUNNING);
    if (ofsc.liveness_check())
      c->add_timed_callback(send_echo, ofsc.echo_interval() * 1000, cc);
    connection_callback(cc, OFConnection::EVENT_ESTABLISHED);

    goto dispatch;
  }

  // Handle version negotiation failing
  if (ofsc.handshake() and cc->get_state() == OFConnection::STATE_HANDSHAKE and
      type == OFPT_ERROR) {
    cc->close();
    cc->set_state(OFConnection::STATE_FAILED);
    connection_callback(cc, OFConnection::EVENT_FAILED_NEGOTIATION);

    if (ofsc.dispatch_all_messages())
      goto dispatch;
    else
      goto done;
  }
  goto dispatch;

// Dispatch a message to the user callback and goto done
dispatch:
  message_callback(cc, type, data, len);
  if (this->ofsc.keep_data_ownership()) this->free_data(data);
  return;

// Free the message (if necessary) and return
done:
  this->free_data(data);
  return;
}

void OFServer::free_data(void* data) { BaseOFServer::free_data(data); }

void OFServer::base_connection_callback(BaseOFConnection* c,
                                        BaseOFConnection::Event event_type) {
  // If the connection was closed, destroy it
  // (BaseOFServer::base_connection_callback will do it for us).
  // There's no need to notify the user, since a BaseOFConnection::EVENT_DOWN
  // event already means a BaseOFConnection::EVENT_CLOSED will happen and
  // nothing should be expected from the connection anymore.
  if (event_type == BaseOFConnection::EVENT_CLOSED) {
    BaseOFServer::base_connection_callback(c, event_type);
    // TODO: delete the OFConnection? Currently we keep track of all
    // connections that have been started and their status. When a
    // connection is closed, pretty much all of its data is freed already,
    // so this isn't a big overhead, and so we keep the references to old
    // connections for the user.
    return;
  }

  OFConnection* cc;
  int conn_id = c->get_id();
  if (event_type == BaseOFConnection::EVENT_UP) {
    if (ofsc.handshake()) {
      int msglen = 8;
      if (this->ofsc.max_supported_version() >= 4 &&
          ofsc.use_hello_elements()) {
        msglen = 16;
      }

      uint8_t msg[msglen];

      struct ofp_hello* hello = (struct ofp_hello*)&msg;
      hello->header.version = this->ofsc.max_supported_version();
      hello->header.type = OFPT_HELLO;
      hello->header.length = htons(msglen);
      hello->header.xid = htonl(HELLO_XID);

      if (this->ofsc.max_supported_version() >= 4 &&
          ofsc.use_hello_elements()) {
        struct ofp_hello_elem_versionbitmap* elm =
            (struct ofp_hello_elem_versionbitmap*)(&msg[8]);
        elm->type = htons(OFPHET_VERSIONBITMAP);
        elm->length = htons(8);

        uint32_t* bitmaps = (uint32_t*)(&msg[12]);
        *bitmaps = htonl(*this->ofsc.supported_versions());
      }

      c->send(&msg, msglen);
    }

    cc = new OFConnection(c, this);
    lock_ofconnections();
    ofconnections[conn_id] = cc;
    unlock_ofconnections();

    connection_callback(cc, OFConnection::EVENT_STARTED);
  } else if (event_type == BaseOFConnection::EVENT_DOWN) {
    cc = get_ofconnection(conn_id);
    connection_callback(cc, OFConnection::EVENT_CLOSED);
    cc->close();
  }
}

/** This method will periodically send echo requests. */
void* OFServer::send_echo(void* arg) {
  OFConnection* cc = static_cast<OFConnection*>(arg);

  if (!cc->is_alive()) {
    cc->close();
    cc->get_ofhandler()->connection_callback(cc, OFConnection::EVENT_DEAD);
    return NULL;
  }

  uint8_t msg[8];
  memset((void*)msg, 0, 8);
  msg[0] = (uint8_t)cc->get_version();
  msg[1] = OFPT_ECHO_REQUEST;
  ((uint16_t*)msg)[1] = htons(8);
  ((uint32_t*)msg)[1] = htonl(ECHO_XID);

  cc->set_alive(false);
  cc->send(msg, 8);

  return NULL;
}

}  // namespace fluid_base
