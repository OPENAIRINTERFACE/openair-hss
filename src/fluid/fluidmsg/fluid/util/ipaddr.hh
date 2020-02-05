#ifndef __IPADDRESS_H__
#define __IPADDRESS_H__

#include <arpa/inet.h>
#include <net/if.h>
#include <stdint.h>
#include <string.h>
#include <sstream>
#include <string>

namespace fluid_msg {

enum { NONE = 0, IPV4 = 4, IPV6 = 6 };

class IPAddress {
 public:
  IPAddress();
  IPAddress(const char* address);
  IPAddress(const std::string& address);
  IPAddress(const IPAddress& other);
  IPAddress(const uint32_t ip_addr);
  IPAddress(const uint8_t ip_addr[16]);
  IPAddress(const struct in_addr& ip_addr);
  IPAddress(const struct in6_addr& ip_addr);
  ~IPAddress(){};

  IPAddress& operator=(const IPAddress& other);
  bool operator==(const IPAddress& other) const;
  int get_version() const;
  void setIPv4(uint32_t address);
  void setIPv6(uint8_t address[16]);
  uint32_t getIPv4();
  uint8_t* getIPv6();
  static uint32_t IPv4from_string(const std::string& address);
  static struct in6_addr IPv6from_string(const std::string& address);

 private:
  int version;
  union {
    uint32_t ipv4;
    uint8_t ipv6[16];
  };
};
}  // namespace fluid_msg
#endif /* __IPADDRESS_H__ */
