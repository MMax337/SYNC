#pragma once

#include "socket.hpp"
#include "common.hpp"
#include <unordered_set>


class Node {
 public:
  Node(const std::optional<std::string>& bind_address, const uint16_t port, std::optional<PeerID> peer);
  ~Node() = default;

  void run();
 private:
  static constexpr auto SYNC_START_DELAY = std::chrono::seconds(2); // Delay after becoming a leader.
  static constexpr auto SYNC_INTERVAL    = std::chrono::seconds(5);
  static constexpr auto SYNC_TIMEOUT     = std::chrono::seconds(20);


  static auto constexpr SYNC_TIME = 5;
  struct SyncInfo {
    timestamp_t T1;
    timestamp_t T2;
    timestamp_t T3;
    timestamp_t T4;

    PeerID master;
    sync_level_t master_lvl;
    bool active = false;
  };

  Socket socket;
  sync_level_t syncLevel = SYNC_LEVEL_UNSYNCED;
  std::optional<PeerID> helloPeer = std::nullopt;
  std::optional<PeerID> syncedWith = std::nullopt;
  
  peer_set_t peers;
  peer_set_t ack_required_peers; // Peers to whom the CONNECT was sent and waiting for ACK_CONNECT
  timestamp_t offsetMs = 0;
  SyncInfo syncInfo;
  const TimePoint bootTime;

  TimePoint lastSyncSent; // The time without offset.
  TimePoint lastGoodSync;
  TimePoint leaderStart; // when a node became a leader.

  void sendHello(const PeerID& target);
  void sendHelloReply(const PeerID& target);
  void sendConnect(const PeerID& target);
  void sendAckConnect(const PeerID& target);
  void sendSyncStart();
  void sendTime(const PeerID& target);

  void handleHelloReply(const message_t& msg, const PeerID& from);
  void handleAckConnect(const message_t& msg, const PeerID& from);
  void handleSyncStart(const Socket::ReceivedMessage& msg);
  void handleLeader(const message_t& msg);
  void handleDelayRequest(const message_t& msg, const PeerID& from);
  void handleDelayResponse(const Socket::ReceivedMessage& msg);

  void handleMessage(const Socket::ReceivedMessage& msg);

  void becomeLeader();
  void stopBeingLeader();

  bool isLeader();
  void becomeUnsync();

  // Time including offset
  timestamp_t now();
  timestamp_t toTimestamp(TimePoint t);
  std::chrono::milliseconds diff(TimePoint a, TimePoint b);

  template<typename arg_t, typename... args_t>
  void log(arg_t&& arg, args_t&&... args) {
    if constexpr (enable_logging) {
      auto myTime = diff(Clock::now(), bootTime);
      std::cout << "At local/offset " << myTime << "/" << now() << " ms. "
      << std::forward<arg_t>(arg);
      // Fold expression for remaining arguments
      ((std::cout << std::forward<args_t>(args)), ...);
      std::cout << '\n';
    }
  }

};