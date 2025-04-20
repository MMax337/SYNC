#include "peer_id.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>

PeerID::PeerID(const std::string& ip_str, peer_port_t port) : port(port) {
  in_addr addr;
  if (inet_pton(AF_INET, ip_str.c_str(), &addr) != 1) {
    throw std::invalid_argument("Invalid IP address: " + ip_str);
  }
  ip = ntohl(addr.s_addr); // convert from network to host byte order
}

std::size_t PeerID::Hash::operator()(const PeerID& pid) const {
  return std::hash<peer_ip_t>()(pid.ip) ^ std::hash<peer_port_t>()(pid.port);
}

std::string PeerID::to_string() const {
  std::ostringstream oss;
  oss << *this;
  return oss.str();
}

std::ostream& operator<<(std::ostream& os, const PeerID& pid) {
  in_addr addr;
  addr.s_addr = htonl(pid.ip);

  char str[INET_ADDRSTRLEN];

  if (inet_ntop(AF_INET, &addr, str, sizeof(str)) != NULL) {
    os << str << ':' << pid.port;
  } else {
    os << "[invalid ip]:" << pid.port;
  }
  return os;
}

