#include "ipaddr.hh"
#include <stdio.h>
#include <stdlib.h>

namespace fluid_msg {

IPAddress::IPAddress() : version(NONE) { memset(&ipv6, 0x0, 16); }

IPAddress::IPAddress(const char *address) {
  std::string saddress(address);
  if (saddress.find('.') != std::string::npos) {
    this->version = IPV4;
    this->ipv4 = IPv4from_string(address);
  } else if (saddress.find(':') != std::string::npos) {
    this->version = IPV6;
    struct in6_addr addr = IPv6from_string(address);
    memcpy(this->ipv6, &addr, 16);
  }
}

IPAddress::IPAddress(const std::string &address) {
  if (address.find('.') != std::string::npos) {
    this->version = IPV4;
    this->ipv4 = IPv4from_string(address);
  } else if (address.find(':') != std::string::npos) {
    this->version = IPV6;
    struct in6_addr addr = IPv6from_string(address);
    memcpy(this->ipv6, &addr, 16);
  }
}

IPAddress::IPAddress(const IPAddress &other) : version(other.version) {
  if (this->version == IPV4) {
    this->ipv4 = other.ipv4;
  } else {
    memcpy(&this->ipv6, &other.ipv6, 16);
  }
}

IPAddress::IPAddress(const uint32_t ip_addr) : version(IPV4), ipv4(ip_addr) {}

IPAddress::IPAddress(const uint8_t ip_addr[16]) : version(IPV6) {
  memcpy(this->ipv6, ip_addr, 16);
}

IPAddress::IPAddress(const struct in_addr &ip_addr)
    : version(IPV4), ipv4(ip_addr.s_addr) {}

IPAddress::IPAddress(const struct in6_addr &ip_addr) : version(IPV6) {
  memcpy(&ipv6, &ip_addr, sizeof(struct in6_addr));
}

IPAddress &IPAddress::operator=(const IPAddress &other) {
  if (this != &other) {
    this->version = other.version;
    if (this->version == IPV4) {
      this->ipv4 = other.ipv4;
    } else {
      memcpy(&this->ipv6, &other.ipv6, 16);
    }
  }
  return *this;
}

bool IPAddress::operator==(const IPAddress &other) const {
  if (this->version == IPV4 && other.version == IPV4) {
    return (this->ipv4 == other.ipv4);
  } else {
    if (this->version == IPV6 && other.version == IPV6) {
      return memcmp(other.ipv6, &other.ipv6, 16);
    }
  }
  return false;
}

int IPAddress::get_version() const { return this->version; }

void IPAddress::setIPv4(uint32_t address) {
  this->version = IPV4;
  this->ipv4 = address;
}

void IPAddress::setIPv6(uint8_t address[16]) {
  this->version = IPV6;
  memcpy(this->ipv6, address, 16);
}

uint32_t IPAddress::getIPv4() { return this->ipv4; }

uint8_t *IPAddress::getIPv6() { return this->ipv6; }

uint32_t IPAddress::IPv4from_string(const std::string &address) {
  struct in_addr n;
  int pos = address.find('/');
  if (pos != std::string::npos) {
    int prefix = atoi(address.substr(pos + 1).c_str());
    std::string ip = address.substr(0, pos);
    inet_pton(AF_INET, ip.c_str(), &n);
    for (int i = prefix; i < 32; i++) {
      n.s_addr &= ~(1 << i);
    }
  } else {
    inet_pton(AF_INET, address.c_str(), &n);
  }
  return n.s_addr;
}

struct in6_addr IPAddress::IPv6from_string(const std::string &address) {
  struct in6_addr n6;
  inet_pton(AF_INET6, address.c_str(), &n6);
  return n6;
}

}  // namespace fluid_msg
