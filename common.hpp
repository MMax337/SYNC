#pragma once

#include <string>
#include <chrono>
#include <unordered_set>
#include <optional>
#include <cstdint>
#include <vector>
#include <netinet/in.h>

struct PeerID;

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
using peer_set_t      = std::unordered_set<PeerID, PeerID::Hash>;

constexpr sync_level_t SYNC_LEVEL_UNSYNCED = 255;
constexpr sync_level_t SYNC_LEVEL_LEADER = 0;
constexpr sync_level_t MAX_SYNC_LEVEL = 254;
constexpr size_t MAX_DATAGRAM = std::numeric_limits<uint16_t>::max() - 8 - 20;

struct PeerID {
  uint32_t ip;    // IPv4 address in host order.
  uint16_t port;  // Port in host order.

  bool operator==(const PeerID& other) const {
    return ip == other.ip && port == other.port;
  }

  struct Hash {
    std::size_t operator()(const PeerID& pid) const {
      return std::hash<uint32_t>()(pid.ip) ^ std::hash<uint16_t>()(pid.port);
    }
  };
};

