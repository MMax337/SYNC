#pragma once

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <cstring>
#include <arpa/inet.h>
#include "common.hpp"

namespace Message {
  enum class Type : message_type_t {
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

  Type type(const message_t& data);

  message_t makeHello();
  message_t makeHelloReply(const peer_set_t& peers);
  message_t makeConnect();
  message_t makeAckConnect();
  message_t makeSyncStart(sync_level_t sync, timestamp_t T1);
  message_t makeDelayRequest();
  message_t makeDelayResponse(sync_level_t sync, timestamp_t T4);
  message_t makeLeader();
  message_t makeGetTime();
  message_t makeTime(sync_level_t sync, timestamp_t timestamp);

  peer_set_t parseHelloReply(const message_t& msg);
  std::pair<sync_level_t, timestamp_t> parseSyncStart(const message_t& msg);
  std::pair<sync_level_t, timestamp_t> parseDelayResponse(const message_t& msg);
  sync_level_t parseLeader(const message_t& msg);

  void logError(const message_t& msg);
};
