#include <iostream>
#include <string>
#include <optional>
#include <cstdlib>

bool is_valid_port(const std::string& s, bool allow_zero = true) {
  try {
    int port = std::stoi(s);
    return allow_zero ? (port >= 0 && port <= 65535)
                      : (port >= 1 && port <= 65535);
  } catch (...) {
    return false;
  }
}

int main(int argc, char* argv[]) {
  int port = 0;

  std::optional<std::string> bind_address;
  std::optional<std::string> peer_address;
  std::optional<int> peer_port;

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
  if (peer_address.has_value() != !peer_port.has_value()) {
    std::cerr << "Both -a and -r must be provided together.\n";
    return 1;
  }

  // Wyświetlenie konfiguracji
  std::cout << "Bind address: " << (bind_address ? *bind_address : "INADDR_ANY (default)") << "\n";
  std::cout << "Port: " << port << "\n";

  if (peer_address && peer_port) {
      std::cout << "Peer address: " << *peer_address << "\n";
      std::cout << "Peer port: " << *peer_port << "\n";
  } else {
      std::cout << "No peer configured.\n";
  }

  return 0;
}
