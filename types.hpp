#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <chrono>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

using timestamp_t     = std::int64_t;         // Logical timestamp in ms since node boot.
using sync_level_t    = std::uint8_t;         // Synchronization level.
using address_t       = std::uint32_t;        // IPv4 address (host byte order)
using address_len_t   = std::uint8_t;         // Address length.
using port_t          = std::uint16_t;        // UDP port number (host byte order)
using message_type_t  = std::uint8_t;         // Message type identifier
using peer_count_t    = std::uint16_t;        // Number of peers in HELLO_REPLY.
using message_t       = std::vector<std::uint8_t>;

inline constexpr sync_level_t SYNC_LEVEL_UNSYNCED = 255;
inline constexpr sync_level_t SYNC_LEVEL_LEADER = 0;
inline constexpr sync_level_t MAX_SYNC_LEVEL = 254;

inline constexpr size_t MAX_DATAGRAM_SIZE = std::numeric_limits<uint16_t>::max();
// Max number of bytes that can be put into UDP datagram in IPv4
// - 8  for UDP header
// - 20 for IP header 
inline constexpr size_t MAX_DATAGRAM_PAYLOD = MAX_DATAGRAM_SIZE - 8 - 20;