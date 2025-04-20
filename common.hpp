#pragma once

#include <string>
#include <chrono>
#include <unordered_set>
#include <optional>
#include <cstdint>
#include <vector>
#include <netinet/in.h>
#include <iostream>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

using timestamp_t     = std::int64_t;         // Logical timestamp in ms since node boot
using sync_level_t    = std::uint8_t;         // Synchronization level (0 = unsynced)
using peer_ip_t       = std::uint32_t;        // IPv4 address (network byte order)
using peer_port_t     = std::uint16_t;        // UDP port number (network byte order)
using message_type_t  = std::uint8_t;         // Message type identifier
using peer_count_t    = std::uint16_t;        // Number of peers in HELLO_REPLY (network byte order)
using peer_addr_len_t = std::uint8_t;
using message_t       = std::vector<std::uint8_t>;

constexpr sync_level_t SYNC_LEVEL_UNSYNCED = 255;
constexpr sync_level_t SYNC_LEVEL_LEADER = 0;
constexpr sync_level_t MAX_SYNC_LEVEL = 254;

// Max number of bytes that can be put into UDP datagram in IPv4
// - 8  for UDP header
// - 20 for IP header 
constexpr size_t MAX_DATAGRAM = std::numeric_limits<uint16_t>::max() - 8 - 20;

#ifndef NDEBUG
#define NDEBUG true
#endif

inline void errorLog(const std::string& err) {
  std::cerr << "ERROR " << err << '\n';
}

inline void log(const std::string& msg) {
  if constexpr (!NDEBUG) {
    std::cout << msg;
  }
}


struct PeerID {
  peer_ip_t ip;  
  peer_port_t port;

  bool operator==(const PeerID& other) const {
    return ip == other.ip && port == other.port;
  }

  struct Hash {
    std::size_t operator()(const PeerID& pid) const {
      return std::hash<peer_ip_t>()(pid.ip) ^ std::hash<peer_port_t>()(pid.port);
    }
  };
};

using peer_set_t = std::unordered_set<PeerID, PeerID::Hash>;
