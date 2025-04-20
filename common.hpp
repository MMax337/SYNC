#pragma once

#include "types.hpp"
#include "peer_id.hpp"

#include <string>
#include <iostream>
#include <unordered_set>

#ifndef NDEBUG
#define NDEBUG true
#endif

using peer_set_t = std::unordered_set<PeerID, PeerID::Hash>;

inline void error(const std::string& err) {
  std::cerr << "ERROR " << err << '\n';
}

inline void log(const std::string& msg) {
  if constexpr (!NDEBUG) {
    std::cout << msg;
  }
}

