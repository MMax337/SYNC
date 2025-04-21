#pragma once

#include "types.hpp"
#include "peer_id.hpp"

#include <string>
#include <iostream>
#include <unordered_set>

// TODO: change
#ifdef NDEBUG
inline constexpr bool enable_logging = false;
#else
inline constexpr bool enable_logging = true;
#endif

using peer_set_t = std::unordered_set<PeerID, PeerID::Hash>;

template<typename arg_t, typename... args_t>
void error(arg_t&& arg, args_t&&... args) {
  if constexpr (enable_logging) {
    std::cerr << "ERROR " << std::forward<arg_t>(arg);
    // Fold expression for remaining arguments
    ((std::cerr << ", " << std::forward<args_t>(args)), ...);
    std::cerr << '\n';
  }
}

