#pragma once

#include "types.hpp"
#include "peer_id.hpp"

#include <string>
#include <iostream>
#include <unordered_set>
#include <atomic>

#ifdef DEBUG
inline constexpr bool enableLogging = true;
#else
inline constexpr bool enableLogging = false;
#endif


inline std::atomic<bool> stop{false};

using peer_set_t = std::unordered_set<PeerID, PeerID::Hash>;

template<typename arg_t, typename... args_t>
void error(arg_t&& arg, args_t&&... args) {
  std::cerr << "ERROR " << std::forward<arg_t>(arg);
  // Fold expression for remaining arguments
  ((std::cerr << std::forward<args_t>(args)), ...);
  std::cerr << '\n';
}

