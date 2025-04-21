#include "peer_id.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

PeerID::PeerID(const std::string& ip_str, peer_port_t port) : port(port) {
  addrinfo hints {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  addrinfo* res;
  int err = getaddrinfo(ip_str.c_str(), nullptr, &hints, &res);

  if (err != 0) {
    freeaddrinfo(res);
    throw std::invalid_argument("Failed to resolve IP/hostname: " + ip_str +
                                " (" + gai_strerror(err) + ")");
  }

  // Cast to sockaddr_in and extract address
  sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(res->ai_addr);
  ip = ntohl(ipv4->sin_addr.s_addr); 

  freeaddrinfo(res);
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

