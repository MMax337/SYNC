#include <iostream>
#include <string>
#include <optional>
#include <cstdlib>
#include <csignal>

#include "node.hpp"
                      
static bool is_valid_port(const std::string& s, bool allow_zero = true) {
  try {
    int port = std::stoi(s);
    return allow_zero ? (port >= 0 && port <= std::numeric_limits<port_t>::max())
                      : (port >  0 && port <= std::numeric_limits<port_t>::max());
  } catch (...) {
    return false;
  }
}

static void signal_handler(int) {
  stop_requested.store(true, std::memory_order_relaxed);
}

int main(int argc, char* argv[]) {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  int port = 0;

  std::optional<std::string> bind_address = std::nullopt;
  std::optional<std::string> peer_address = std::nullopt;
  std::optional<port_t>      peer_port    = std::nullopt;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-b" && i + 1 < argc) {
      bind_address = argv[++i];
    } else if (arg == "-p" && i + 1 < argc) {
      std::string val = argv[++i];
      if (is_valid_port(val)) {
        port = std::stoi(val);
      } else {
        std::cerr << "Invalid port value for -p: " << val << "\n";
        return 1;
      }
    } else if (arg == "-a" && i + 1 < argc) {
      peer_address = argv[++i];
    } else if (arg == "-r" && i + 1 < argc) {
      std::string val = argv[++i];
      if (is_valid_port(val, false)) {
        peer_port = std::stoi(val);
      } else {
        std::cerr << "Invalid peer port value for -r: " << val << "\n";
        return 1;
      }
    } else {
      std::cerr << "Unknown or incomplete argument: " << arg 
                << " at " << i << "-th position\n";
      return 1;
    }
  }

  // Validate the pair -a/-r
  if (peer_address.has_value() != peer_port.has_value()) {
    std::cerr << "Both -a and -r must be provided together.\n";
    return 1;
  }

  std::optional<PeerID> peer = std::nullopt;
  if (peer_address.has_value() && peer_port.has_value()) {
    peer = PeerID(peer_address.value(), peer_port.value());
  }
  
  try {
    Node node = Node(bind_address, port, peer);
    node.run();
  } catch (...) {
    return 1;
  }

  return 0;
}
