#include <iostream>
#include <string>
#include <optional>
#include <cstdlib>
#include <csignal>

#include "node.hpp"
                      
static bool is_valid_port(const std::string& s, bool allowZero = true) {
  try {
    int port = std::stoi(s);

    return allowZero ? port >= 0 && port <= std::numeric_limits<port_t>::max()
                     : port > 0  && port <= std::numeric_limits<port_t>::max();
  } catch (...) {
    return false;
  }
}

static void signal_handler(int) {
  STOP.store(true, std::memory_order_relaxed);
}

int main(int argc, char* argv[]) {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  port_t port = 0;

  std::optional<std::string> bind_address = std::nullopt;
  std::optional<std::string> peer_address = std::nullopt;
  std::optional<port_t>      peer_port    = std::nullopt;

  bool parseFail = false;

  for (int i = 1; i < argc && !parseFail; ++i) {
    std::string arg = argv[i];

    if (arg == "-b" && i + 1 < argc) {
      bind_address = argv[++i];
    } else if (arg == "-p" && i + 1 < argc) {
      std::string val = argv[++i];
      if (is_valid_port(val)) {
        port = static_cast<port_t>(std::stoul(val));
      } else {
        error("Invalid port value for -p: ", val);
        parseFail = true;
      }
    } else if (arg == "-a" && i + 1 < argc) {
      peer_address = argv[++i];
    } else if (arg == "-r" && i + 1 < argc) {
      std::string val = argv[++i];
      if (is_valid_port(val, false)) {
        peer_port = static_cast<port_t>(std::stoul(val));
      } else {
        error("Invalid peer port value for -r: ", val);
        parseFail = true;
      }
    } else {
      error("Unknown or incomplete argument: ", arg, " at ", i, "-th position");
      parseFail = true;
    }
  }

  if (parseFail) {
    return ERROR_EXIT_CODE;
  }

  // Validate the pair -a/-r
  if (peer_address.has_value() != peer_port.has_value()) {
    error("Both -a and -r must be provided together.");
    return ERROR_EXIT_CODE;
  }
  
  try {
    if (bind_address.has_value() && bind_address.value() == "localhost") {
      throw std::invalid_argument("Bind address must an address not a name");
    }
    
    std::optional<PeerID> peer = std::nullopt;
    if (peer_address.has_value() && peer_port.has_value()) {
      peer = PeerID(peer_address.value(), peer_port.value());
    }

    Node node = Node(bind_address, port, peer);
    node.run();
  } catch (...) {
    return ERROR_EXIT_CODE;
  }

  return 0;
}
