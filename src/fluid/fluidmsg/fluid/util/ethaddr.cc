#include "ethaddr.hh"
#include <stdio.h>

namespace fluid_msg {

EthAddress::EthAddress() : data() {}

EthAddress::EthAddress(const char* address) {
  std::string sadress(address);
  memcpy(this->data, data_from_string(address), IFHWADDRLEN);
}

EthAddress::EthAddress(const std::string& address) {
  memcpy(this->data, data_from_string(address), IFHWADDRLEN);
}

EthAddress::EthAddress(const uint8_t* data) {
  memcpy(this->data, data, IFHWADDRLEN);
}

EthAddress::EthAddress(const EthAddress& other) {
  memcpy(this->data, other.data, IFHWADDRLEN);
}

EthAddress& EthAddress::operator=(const EthAddress& other) {
  if (this != &other) {
    memcpy(this->data, other.data, IFHWADDRLEN);
  }
  return *this;
}

bool EthAddress::operator==(const EthAddress& other) const {
  return memcmp(other.data, this->data, IFHWADDRLEN) == 0;
}

std::string EthAddress::to_string() const {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < IFHWADDRLEN; i++) {
    ss << std::setw(2) << (int)data[i];
    if (i < IFHWADDRLEN - 1) ss << ':';
  }

  return ss.str();
}
void EthAddress::set_data(uint8_t* array) {
  memcpy(this->data, array, IFHWADDRLEN);
}

uint8_t* EthAddress::data_from_string(const std::string& address) {
  static uint8_t data[6];
  char sc;
  int byte;
  std::stringstream ss(address);
  ss << std::hex;
  for (int i = 0; i < IFHWADDRLEN; i++) {
    ss >> byte;
    ss >> sc;
    data[i] = (uint8_t)byte;
  }
  return data;
}
}  // namespace fluid_msg
