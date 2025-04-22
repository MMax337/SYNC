#include "types.hpp"

struct PeerID {
  peer_ip_t ip;     // in host order
  peer_port_t port; // in host order

  PeerID() = default;
  PeerID(peer_ip_t ip, peer_port_t port) : ip(ip), port(port) {}

  PeerID(const std::string& ip_str, peer_port_t port);

  bool operator==(const PeerID& other) const {
    return ip == other.ip && port == other.port;
  }

  std::string to_string() const;

  friend std::ostream& operator<<(std::ostream& os, const PeerID& pid);
  
  struct Hash {
    std::size_t operator()(const PeerID& pid) const;
  };
};

