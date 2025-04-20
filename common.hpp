#pragma once

#include "types.hpp"
#include "peer_id.hpp"

#include <string>
#include <iostream>
#include <unordered_set>


#ifndef NDEBUG
inline constexpr bool enable_logging = false;
#else
inline constexpr bool enable_logging = true;
#endif

using peer_set_t = std::unordered_set<PeerID, PeerID::Hash>;

inline std::ostream& error() {
  std::cerr << "ERROR ";
  return std::cerr;
}

inline void error(const std::string& err) {
  std::cerr << "ERROR " << err;
}

template<typename arg_t, typename... args_t>
void log(arg_t&& arg, args_t&&... args) {
  if constexpr (enable_logging) {
    std::cout << std::forward<arg_t>(arg);
    // Fold expression for remaining arguments
    ((std::cout << ", " << std::forward<args_t>(args)), ...);
    std::cout << '\n';
  }
}

