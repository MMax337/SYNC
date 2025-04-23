#include "types.hpp"

struct PeerID {
  address_t ip;     // in host order
  port_t port; // in host order

  PeerID() = default;
  // Both `ip` and `port` expected in host order.
  PeerID(address_t ip, port_t port) : ip(ip), port(port) {}

  PeerID(const std::string& ip_str, port_t port);

  bool operator==(const PeerID& other) const {
    return ip == other.ip && port == other.port;
  }

  std::string to_string() const;

  friend std::ostream& operator<<(std::ostream& os, const PeerID& pid);
  
  struct Hash {
    std::size_t operator()(const PeerID& pid) const;
  };
};

