#ifndef __MACADDRESS_H__
#define __MACADDRESS_H__

#include <net/if.h>
#include <stdint.h>
#include <string.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace fluid_msg {

class EthAddress {
 public:
  EthAddress();
  EthAddress(const char* address);
  EthAddress(const std::string& address);
  EthAddress(const EthAddress& other);
  EthAddress(const uint8_t* data);

  EthAddress& operator=(const EthAddress& other);
  bool operator==(const EthAddress& other) const;
  std::string to_string() const;
  void set_data(uint8_t* array);
  uint8_t* get_data() { return this->data; }
  static uint8_t* data_from_string(const std::string& address);

 private:
  uint8_t data[6];
  // void data_from_string(const std::string &address);
};
}  // namespace fluid_msg
#endif /* __MACADDRESS_H__ */
