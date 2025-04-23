#pragma once

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <cstring>
#include <arpa/inet.h>
#include "common.hpp"

class Message {
 public:
  enum class Type : uint8_t {
    HELLO = 1,
    HELLO_REPLY = 2,
    CONNECT = 3,
    ACK_CONNECT = 4,
    SYNC_START = 11,
    DELAY_REQUEST = 12,
    DELAY_RESPONSE = 13,
    LEADER = 21,
    GET_TIME = 31,
    TIME = 32,
  };

  static Type type(const message_t& data);

  static message_t makeHello();
  static message_t makeHelloReply(const peer_set_t& peers);
  static message_t makeConnect();
  static message_t makeAckConnect();
  static message_t makeSyncStart(sync_level_t sync, timestamp_t T1);
  static message_t makeDelayRequest();
  static message_t makeDelayResponse(sync_level_t sync, timestamp_t T4);
  static message_t makeLeader();
  static message_t makeGetTime();
  static message_t makeTime(sync_level_t sync, timestamp_t timestamp);

  static peer_set_t parseHelloReply(const message_t& msg);
  static std::pair<sync_level_t, timestamp_t> parseSyncStart(const message_t& msg);
  static std::pair<sync_level_t, timestamp_t> parseDelayResponse(const message_t& msg);
  static sync_level_t parseLeader(const message_t& msg);

  static void logError(const message_t& msg);

 private:
  static constexpr size_t MAX_ERROR_BYTES = 10;
  // MAX_PEERS calculates the maximum number of peers that can fit in the message, 
  // based on the message structure and size constraints.
  static constexpr uint16_t MAX_PEERS = 
      (MAX_DATAGRAM_PAYLOD - sizeof(message_type_t) - sizeof(peer_count_t)) / 
      (sizeof(address_len_t) + sizeof(address_t) + sizeof(port_t));

  static std::pair<sync_level_t, timestamp_t> parseTimeSyncMessage(const message_t& msg, Type expected);
  static uint8_t toByte(Type t);
  static void add_peer(message_t& msg, const PeerID& peer);
  static void add_time(message_t& msg, timestamp_t host_time);
};
