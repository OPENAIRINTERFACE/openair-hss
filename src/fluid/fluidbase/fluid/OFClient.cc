#include "OFClient.hh"
#include "fluid/base/of.hh"

namespace fluid_base {

OFClient::OFClient(int id, std::string address, int port,
                   const struct OFServerSettings ofsc)
    : BaseOFClient(id, address, port) {
  this->ofsc = ofsc;
  this->conn = NULL;
}

OFClient::~OFClient() {
  if (conn != NULL) delete conn;
}

bool OFClient::start(bool block) { return BaseOFClient::start(block); }

void OFClient::start_conn() { BaseOFClient::start_conn(); }

void OFClient::stop_conn() {
  if (conn != NULL) conn->close();
}

void OFClient::stop() {
  stop_conn();
  BaseOFClient::stop();
}

void OFClient::base_message_callback(BaseOFConnection* c, void* data,
                                     size_t len) {
  uint8_t type = ((uint8_t*)data)[1];
  OFConnection* cc = (OFConnection*)c->get_manager();

  // We trust that the other end is using the negotiated protocol
  // version. Should we?

  if (ofsc.liveness_check() and type == OFPT_ECHO_REQUEST) {
    uint8_t msg[8];
    memset((void*)msg, 0, 8);
    msg[0] = ((uint8_t*)data)[0];
    msg[1] = OFPT_ECHO_REPLY;
    ((uint16_t*)msg)[1] = htons(8);
    ((uint32_t*)msg)[1] = ((uint32_t*)data)[1];
    // TODO: copy echo data
    c->send(msg, 8);

    if (ofsc.dispatch_all_messages())
      goto dispatch;
    else
      goto done;
  }

  if (ofsc.handshake() and type == OFPT_HELLO) {
    uint8_t version = ((uint8_t*)data)[0];
    if (not this->ofsc.supported_versions() & (1 << (version - 1))) {
      uint8_t msg[12];
      memset((void*)msg, 0, 8);
      msg[0] = version;
      msg[1] = OFPT_ERROR;
      ((uint16_t*)msg)[1] = htons(12);
      ((uint32_t*)msg)[1] = ((uint32_t*)data)[1];
      ((uint16_t*)msg)[4] = htons(OFPET_HELLO_FAILED);
      ((uint16_t*)msg)[5] = htons(OFPHFC_INCOMPATIBLE);
      cc->send(msg, 12);
      cc->close();
      cc->set_state(OFConnection::STATE_FAILED);
      connection_callback(cc, OFConnection::EVENT_FAILED_NEGOTIATION);
    } else {
      if (ofsc.is_controller()) {
        struct ofp_header msg;
        msg.version = ((uint8_t*)data)[0];
        msg.type = OFPT_FEATURES_REQUEST;
        msg.length = htons(8);
        msg.xid = ((uint32_t*)data)[1];
        c->send(&msg, 8);
      }
    }

    if (ofsc.dispatch_all_messages())
      goto dispatch;
    else
      goto done;
  }

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

  goto dispatch;

// Dispatch a message and goto done
dispatch:
  message_callback(cc, type, data, len);
  goto done;
// Free the message and return
done:
  c->free_data(data);
  return;
}

void OFClient::base_connection_callback(BaseOFConnection* c,
                                        BaseOFConnection::Event event_type) {
  /* If the connection was closed, destroy it.
  There's no need to notify the user, since a DOWN event already
  means a CLOSED event will happen and nothing should be expected from
  the connection. */
  if (event_type == BaseOFConnection::EVENT_CLOSED) {
    BaseOFClient::base_connection_callback(c, event_type);
    // TODO: delete the OFConnection?
    return;
  }

  int conn_id = c->get_id();
  if (event_type == BaseOFConnection::EVENT_UP) {
    if (ofsc.handshake()) {
      struct ofp_hello msg;
      msg.header.version = this->ofsc.max_supported_version();
      msg.header.type = OFPT_HELLO;
      msg.header.length = htons(8);
      msg.header.xid = htonl(HELLO_XID);
      c->send(&msg, 8);
    }

    this->conn = new OFConnection(c, this);
    connection_callback(this->conn, OFConnection::EVENT_STARTED);
  } else if (event_type == BaseOFConnection::EVENT_DOWN) {
    connection_callback(this->conn, OFConnection::EVENT_CLOSED);
  }
}

void OFClient::free_data(void* data) { BaseOFClient::free_data(data); }

void* OFClient::send_echo(void* arg) {
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
