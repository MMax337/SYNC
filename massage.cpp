#include "message.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

using Type = Message::Type;

Type Message::type(const message_t& data) {
  return static_cast<Type>(data[0]);
}

message_t Message::makeHello() {
  return {toByte(Type::HELLO)};
}

message_t Message::makeHelloReply(const peer_set_t& peers) {
  if (peers.size() > MAX_PEERS) {
    throw std::invalid_argument("Too many peers for one message");
  }

  message_t msg;
  msg.push_back(toByte(Type::HELLO_REPLY));

  peer_count_t count = htons(static_cast<peer_count_t>(peers.size()));
  msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&count),
                        reinterpret_cast<uint8_t*>(&count) + sizeof(count));
  
  for (const auto& peer : peers) {
    add_peer(msg, peer);
  }

  return msg;
}

peer_set_t Message::parseHelloReply(const message_t& msg) {
  const size_t minSize  = sizeof(message_type_t) + sizeof(peer_count_t);
  const size_t peerInfo = sizeof(peer_addr_len_t) +
                          sizeof(peer_port_t) + sizeof(peer_ip_t);

  if (msg.size() < minSize || Message::type(msg) != Type::HELLO_REPLY) {
    logError(msg);
    throw std::runtime_error("Invalid HELLO_REPLY message");
  }

  size_t offset = 1;  // Skip message type.
  peer_count_t count;
  std::memcpy(&count, &msg[offset], sizeof(count));
  count = ntohs(count);
  offset += sizeof(count);

  peer_set_t peers;
  if ((msg.size() - minSize) % peerInfo != 0) {
    logError(msg);
    throw std::runtime_error("Truncated peer entry in HELLO_REPLY");
  }

  for (peer_count_t i = 0; i < count; ++i) {
    uint8_t ipLength = msg[offset++];
    if (ipLength != sizeof(peer_ip_t)) {
      logError(msg);
      throw std::runtime_error("Invalid IP length in HELLO_REPLY");
    }

    peer_ip_t ip;
    std::memcpy(&ip, &msg[offset], sizeof(ip));
    ip = ntohl(ip);
    offset += sizeof(ip);

    peer_port_t port;
    std::memcpy(&port, &msg[offset], sizeof(port));
    port = ntohs(port);
    offset += sizeof(port);

    peers.insert(PeerID{ip, port});
  }

  return peers;
}

std::pair<sync_level_t, timestamp_t> Message::parseSyncStart(const message_t& msg) {  
  return Message::parseTimeSyncMessage(msg, Type::SYNC_START);
}

std::pair<sync_level_t, timestamp_t> Message::parseDelayResponse(const message_t& msg) {
  return Message::parseTimeSyncMessage(msg, Type::DELAY_RESPONSE);
}

sync_level_t Message::parseLeader(const message_t& msg) {
  const size_t expectedSize = sizeof(message_type_t) + sizeof(sync_level_t);

  if (msg.size() != expectedSize || Message::type(msg) != Type::LEADER ||
      (msg[1] != SYNC_LEVEL_LEADER && msg[1] != SYNC_LEVEL_UNSYNCED)) {
        
    logError(msg);
    throw std::runtime_error("Invalid LEADER message");
  }

  return msg[1];
}

message_t Message::makeConnect() {
  return {toByte(Type::CONNECT)};
}

message_t Message::makeAckConnect() {
  return {toByte(Type::ACK_CONNECT)};
}

message_t Message::makeSyncStart(sync_level_t sync, timestamp_t T1) {
  message_t msg;
  msg.push_back(toByte(Type::SYNC_START));
  msg.push_back(sync);
  add_time(msg, T1);
  return msg;
}

message_t Message::makeDelayRequest() {
  message_t msg;
  msg.push_back(toByte(Type::DELAY_REQUEST));
  return msg;
}

message_t Message::makeDelayResponse(sync_level_t sync, timestamp_t T4) {
  message_t msg;
  msg.push_back(toByte(Type::DELAY_RESPONSE));
  msg.push_back(sync);
  add_time(msg, T4);
  return msg;
}

message_t Message::makeLeader() {
  return {toByte(Type::LEADER)};
}

message_t Message::makeGetTime() {
  return {toByte(Type::GET_TIME)};
}

message_t Message::makeTime(sync_level_t sync, timestamp_t timestamp) {
  message_t msg;
  msg.push_back(toByte(Type::TIME));
  msg.push_back(sync);
  add_time(msg, timestamp);
  return msg;
}

void Message::logError(const message_t& msg) {
  std::ostringstream oss;
  oss << "MSG ";
  if (msg.empty()) {
    oss << "EMPTY";
  } else {
    for (size_t i = 0; i < std::min(msg.size(), MAX_ERROR_BYTES); ++i) {
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg[i]);
    }
  }
  error(oss.str());
}

std::pair<sync_level_t, timestamp_t> Message::parseTimeSyncMessage(const message_t& msg,
                                                                   Type expected) {
  const size_t expectedSize = sizeof(message_type_t) +
                              sizeof(sync_level_t) + sizeof(timestamp_t);

  if (msg.size() != expectedSize || Message::type(msg) != expected) {
    logError(msg);
    throw std::runtime_error("Invalid SYNC_START message");
  }
  
  sync_level_t lvl = msg[1];

  timestamp_t t;
  std::memcpy(&t, &msg[2], sizeof(t));
  t = be64toh(t);

  return {lvl, t};
}

uint8_t Message::toByte(Type t) {
  return static_cast<uint8_t>(t);
}

void Message::add_peer(message_t& msg, const PeerID& peer) {
  peer_ip_t ip = htonl(peer.ip);
  peer_port_t port = htons(peer.port);

  msg.push_back(static_cast<uint8_t>(sizeof(peer_ip_t)));

  msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&ip),
                        reinterpret_cast<uint8_t*>(&ip) + sizeof(ip));

  msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&port),
                        reinterpret_cast<uint8_t*>(&port) + sizeof(port));
}

void Message::add_time(message_t& msg, timestamp_t host_time) {
  timestamp_t netT = htobe64(host_time);
  msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&netT),
                        reinterpret_cast<uint8_t*>(&netT) + sizeof(netT));
}
