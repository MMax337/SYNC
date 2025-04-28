#include "node.hpp"
#include "message.hpp"

#include <iostream>

Node::Node(const std::optional<std::string>& bind_address,
           const port_t port, std::optional<PeerID> peer)
  : socket(bind_address, port), helloPeer(peer), bootTime(Clock::now()) {
  socket.setReadTimeOut(SYNC_INTERVAL);
}

void Node::run() {
  if (helloPeer.has_value()) {
    sendHello(helloPeer.value());
  }

  while (!STOP.load()) {
    auto now = Clock::now();

    // whether the next SYNC_START must be as a leader (after SYNC_START_DELAY).
    bool isLeaderSyncTurn = isLeader() && leaderStart != TimePoint{};

    if (syncInfo.active && diff(now, syncInfo.lastContact) > SYNC_INTERVAL) {
      // Long time without response, stop synchronizing with the node. 
      syncInfo.active = false;
    } 
    
    if (isLeaderSyncTurn && diff(now, leaderStart) > SYNC_START_DELAY) {
      log("Me a leader starts syncing");
      socket.setReadTimeOut(SYNC_INTERVAL);
      leaderStart = TimePoint{};
      sendSyncStart();
    } else if (!isLeaderSyncTurn && syncLevel < MAX_SYNC_LEVEL &&
                diff(now, lastSyncSent) > SYNC_INTERVAL) {
      log("Syncing start");
      sendSyncStart();
    } else if (!isLeader() && diff(now, lastGoodSync) > SYNC_TIMEOUT) {
      log("Long time without sync, becoming unsync");
      becomeUnsync();
    }

    auto msg = socket.recvFrom();
    if (msg.has_value()) {
      try {
        handleMessage(msg.value());
      } catch (std::runtime_error& e) {
        // Ignore the message.
      }
    }
  }
}

void Node::sendHello(const PeerID& target) {
  log("Sending HELLO to ", target);
  auto msg = Message::makeHello();
  socket.sendTo(msg, target);
}

void Node::sendHelloReply(const PeerID& target) {
  if (peers.contains(target)) {
    peers.erase(target);
  }

  log("Sending HELLO_REPLY to ", target, " peers num: ", peers.size());
  try {
    auto msg = Message::makeHelloReply(peers);
    socket.sendTo(msg, target);
  } catch (std::invalid_argument& e) {}
  peers.insert(target);
}

void Node::handleHelloReply(const message_t& msg, const PeerID& from) {
  if (!helloPeer.has_value() || helloPeer.value() != from) {
    Message::logError(msg);
    return;
  }

  peers.insert(from);
  helloPeer = std::nullopt; // set peer to nullopt because HELLO_REPLY is no longer expected.
  
  auto knownPeers = Message::parseHelloReply(msg);
  peer_set_t myAddresses = socket.getBoundAddressAndPort();
  
  bool knownPeerContainsMe = false;
  for (const auto& address : myAddresses) {
    if (knownPeers.contains(address)) {
      knownPeerContainsMe = true;
      break;
    }
  }

  if (knownPeers.contains(from) || knownPeerContainsMe) {
    log("GOT HELLO_REPLY which contains either sender or receiver");
    Message::logError(msg);
    return;
  }

  ack_required_peers = std::move(knownPeers);

  log("Got HELLO_REPLY from: ", from, " peers count: ", ack_required_peers.size());

  for (const auto& peer : ack_required_peers) {
    sendConnect(peer);
  }
}

void Node::sendConnect(const PeerID& target) {
  log("Sending CONNECT to ", target);

  auto msg = Message::makeConnect();
  socket.sendTo(msg, target);
}

void Node::sendAckConnect(const PeerID& target) {
  log("Sending ACK_CONNECT to ", target);
  auto msg = Message::makeAckConnect();
  socket.sendTo(msg, target);

  peers.insert(target);
}

void Node::sendSyncStart() {
  for (const auto& peer : peers) {
    log("Sending SYNC_START to ", peer);
    try {
      auto msg = Message::makeSyncStart(syncLevel, now());
      socket.sendTo(msg, peer);
      auto now = Clock::now();
      lastSyncSent = now;
      syncSent[peer] = now;
    } catch (std::runtime_error& e) {}
  }
}

void Node::handleAckConnect(const message_t& msg, const PeerID& from) {
  if (!ack_required_peers.contains(from)) {
    Message::logError(msg);
    return;
  }

  ack_required_peers.erase(from);
  peers.insert(from);
}

void Node::handleLeader(const message_t& msg) {
  sync_level_t sync = Message::parseLeader(msg);

  if (sync == SYNC_LEVEL_LEADER) {
    becomeLeader();
  } else if (isLeader() && sync == SYNC_LEVEL_UNSYNCED) {
    stopBeingLeader();
  } else {
    Message::logError(msg);
  }
}

void Node::handleSyncStart(const Socket::ReceivedMessage& msg) {
  auto& [from, data, T2] = msg;
  
  auto [lvl, T1] = Message::parseSyncStart(data);

  bool mySyncPartner = syncedWith.has_value() && syncedWith.value() == from;

  if (mySyncPartner && lvl < syncLevel) {
    lastGoodSync = T2;
  }

  if (!peers.contains(from) || syncInfo.active || lvl >= MAX_SYNC_LEVEL ||
      (!mySyncPartner && lvl + 2 > syncLevel)) {
    Message::logError(data);
    return;
  }

  if (mySyncPartner && lvl >= syncLevel) {
    log("GOT SYNC_START from node I am synced with: ", from,
        " with the sync level ", static_cast<int>(lvl));
    becomeUnsync();
    return;
  }

  log("Got SYNC_START from: ", from);

  // sync can be done
  syncInfo.active = true;
  syncInfo.master = from;
  syncInfo.master_lvl = lvl;
  syncInfo.T1 = T1;
  syncInfo.T2 = toTimestamp(T2) - offsetMs;

  log("Sending DELAY_REQUEST to ", from);
  auto delayRequest = Message::makeDelayRequest();
  syncInfo.T3 = now();
  syncInfo.lastContact = Clock::now();
  socket.sendTo(delayRequest, from);
}

void Node::becomeLeader() {
  syncLevel = SYNC_LEVEL_LEADER;
  offsetMs = 0;
  syncInfo.active = false;
  syncedWith = std::nullopt;
  leaderStart = Clock::now();
  socket.setReadTimeOut(SYNC_START_DELAY);
  
  log("Became a leader");
}

void Node::stopBeingLeader() {
  log("Stopped being a leader");
  becomeUnsync();
}

void Node::sendTime(const PeerID& target) {
  auto msg = Message::makeTime(syncLevel, now());
  socket.sendTo(msg, target);
}

void Node::handleDelayRequest(const Socket::ReceivedMessage& data) {
  auto& [from, msg, receivedAt] = data;

  log("Got DELAY_REQUEST from: ", from);

  if (!peers.contains(from) || !syncSent.contains(from) ||
      diff(receivedAt, syncSent[from]) > SYNC_INTERVAL) {
    Message::logError(msg);
    return;
  }

  log("Sending DELAY_RESPONSE to: ", from);

  auto response = Message::makeDelayResponse(syncLevel, now());
  socket.sendTo(response, from);
}

void Node::handleDelayResponse(const Socket::ReceivedMessage& msg) {
  auto& [from, data, receivedAt] = msg;
  log("Got DelayResposne, from: ", from);

  if (!syncInfo.active || from != syncInfo.master) {
    Message::logError(data);
    return;
  }

  auto [lvl, T4] = Message::parseDelayResponse(data);

  if (lvl != syncInfo.master_lvl) {
    Message::logError(data);
    return;
  }

  syncInfo.T4 = T4;
  offsetMs += (syncInfo.T2 - syncInfo.T1 + syncInfo.T3 - syncInfo.T4) / 2;

  syncInfo.active = false;
  syncLevel = lvl + 1;
  syncedWith = from;

  log("Synced with: ", from, " offset: ", offsetMs);
}

void Node::handleMessage(const Socket::ReceivedMessage& msg) {
  auto& [from, data, receivedAt] = msg;

  if (data.empty()) {
    Message::logError(data);
    return;
  }

  switch (Message::type(data)) {
    case Message::Type::HELLO:
      log("Got HELLO from: ", from);
      sendHelloReply(from);
      break;
    case Message::Type::HELLO_REPLY:
      handleHelloReply(data, from);
      break;
    case Message::Type::CONNECT:
      log("Got CONNECT from: ", from);
      sendAckConnect(from);
      break;
    case Message::Type::ACK_CONNECT:
      handleAckConnect(data, from);
      break;
    case Message::Type::SYNC_START:
      handleSyncStart(msg);
      break;
    case Message::Type::DELAY_REQUEST:
      handleDelayRequest(msg);
      break;
    case Message::Type::DELAY_RESPONSE:
      handleDelayResponse(msg);
      break;
    case Message::Type::LEADER:
      handleLeader(data);
      break;
    case Message::Type::GET_TIME:
      sendTime(from);
      break;
    default:
      Message::logError(data);
      break;
  }
}

timestamp_t Node::now() {
  auto now = Clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now - bootTime).count() - offsetMs;
}

timestamp_t Node::toTimestamp(TimePoint t) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(t - bootTime).count(); 
}

bool Node::isLeader() {
  return syncLevel == SYNC_LEVEL_LEADER;
}

std::chrono::milliseconds Node::diff(TimePoint a, TimePoint b) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(a - b); 
}

void Node::becomeUnsync() {
  syncLevel = SYNC_LEVEL_UNSYNCED;
  syncedWith = std::nullopt;
  offsetMs = 0;
  lastGoodSync = Clock::now();

  log("Became unsynced");
}