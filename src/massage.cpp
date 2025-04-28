#include "message.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

using Type = Message::Type;

namespace {
  // MAX_PEERS calculates the maximum number of peers that can fit in the message, 
  // based on the message structure and size constraints.
  constexpr peer_count_t MAX_PEERS = 
      (MAX_DATAGRAM_PAYLOD - sizeof(message_type_t) - sizeof(peer_count_t)) / 
      (sizeof(address_len_t) + sizeof(address_t) + sizeof(port_t));

  std::pair<sync_level_t, timestamp_t> parseTimeSyncMessage(const message_t& msg, Type expected) {
    const size_t expectedSize = sizeof(message_type_t) +
    sizeof(sync_level_t) + sizeof(timestamp_t);

    if (msg.size() != expectedSize || Message::type(msg) != expected) {
      Message::logError(msg);
      throw std::runtime_error("Invalid SYNC_START message");
    }

    sync_level_t lvl = msg[1];

    timestamp_t t;
    std::memcpy(&t, &msg[2], sizeof(t));
    t = be64toh(t);

    return {lvl, t};
  }
  
  uint8_t toByte(Type t) {
    return static_cast<uint8_t>(t);
  }

  void add_peer(message_t& msg, const PeerID& peer) {
    address_t ip = htonl(peer.ip);
    port_t port = htons(peer.port);
  
    msg.push_back(static_cast<uint8_t>(sizeof(address_t)));
  
    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&ip),
                          reinterpret_cast<uint8_t*>(&ip) + sizeof(ip));
  
    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&port),
                          reinterpret_cast<uint8_t*>(&port) + sizeof(port));
  }
  
  void add_time(message_t& msg, timestamp_t host_time) {
    timestamp_t netT = htobe64(host_time);
    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&netT),
                          reinterpret_cast<uint8_t*>(&netT) + sizeof(netT));
  }
} // unnamed namespace end


namespace Message {
  Type type(const message_t& data) {
    return static_cast<Type>(data[0]);
  }

  message_t makeHello() {
    return {toByte(Type::HELLO)};
  }

  message_t makeHelloReply(const peer_set_t& peers) {
    message_t msg;
    msg.push_back(toByte(Type::HELLO_REPLY));

    peer_count_t count = htons(static_cast<peer_count_t>(peers.size()));

    if (peers.size() > MAX_PEERS) {
      add_peer(msg, *peers.begin());
      logError(msg);
      throw std::invalid_argument("Too many peers for one message");
    }

    msg.insert(msg.end(), reinterpret_cast<uint8_t*>(&count),
                          reinterpret_cast<uint8_t*>(&count) + sizeof(count));
    
    for (const auto& peer : peers) {
      add_peer(msg, peer);
    }

    return msg;
  }

  peer_set_t parseHelloReply(const message_t& msg) {
    const size_t minSize  = sizeof(message_type_t) + sizeof(peer_count_t);
    const size_t peerInfo = sizeof(address_len_t) +
                            sizeof(port_t) + sizeof(address_t);

    if (msg.size() < minSize || type(msg) != Type::HELLO_REPLY) {
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
      address_len_t ipLength = msg[offset++];
      if (ipLength != sizeof(address_t)) {
        logError(msg);
        throw std::runtime_error("Invalid IP length in HELLO_REPLY");
      }

      address_t ip;
      std::memcpy(&ip, &msg[offset], sizeof(ip));
      ip = ntohl(ip);
      offset += sizeof(ip);

      port_t port;
      std::memcpy(&port, &msg[offset], sizeof(port));
      port = ntohs(port);
      offset += sizeof(port);

      if (port == 0) {
        logError(msg);
        throw std::runtime_error("Zero port in HELLO_REPLY");
      }

      peers.insert(PeerID{ip, port});
    }

    return peers;
  }

  std::pair<sync_level_t, timestamp_t> parseSyncStart(const message_t& msg) {  
    return parseTimeSyncMessage(msg, Type::SYNC_START);
  }

  std::pair<sync_level_t, timestamp_t> parseDelayResponse(const message_t& msg) {
    return parseTimeSyncMessage(msg, Type::DELAY_RESPONSE);
  }

  sync_level_t parseLeader(const message_t& msg) {
    const size_t expectedSize = sizeof(message_type_t) + sizeof(sync_level_t);

    if (msg.size() != expectedSize || type(msg) != Type::LEADER ||
        (msg[1] != SYNC_LEVEL_LEADER && msg[1] != SYNC_LEVEL_UNSYNCED)) {
          
      logError(msg);
      throw std::runtime_error("Invalid LEADER message");
    }

    return msg[1];
  }

  message_t makeConnect() {
    return {toByte(Type::CONNECT)};
  }

  message_t makeAckConnect() {
    return {toByte(Type::ACK_CONNECT)};
  }

  message_t makeSyncStart(sync_level_t sync, timestamp_t T1) {
    message_t msg;
    msg.push_back(toByte(Type::SYNC_START));
    msg.push_back(sync);
    add_time(msg, T1);
    return msg;
  }

  message_t makeDelayRequest() {
    message_t msg;
    msg.push_back(toByte(Type::DELAY_REQUEST));
    return msg;
  }

  message_t makeDelayResponse(sync_level_t sync, timestamp_t T4) {
    message_t msg;
    msg.push_back(toByte(Type::DELAY_RESPONSE));
    msg.push_back(sync);
    add_time(msg, T4);
    return msg;
  }

  message_t makeLeader() {
    return {toByte(Type::LEADER)};
  }

  message_t makeGetTime() {
    return {toByte(Type::GET_TIME)};
  }

  message_t makeTime(sync_level_t sync, timestamp_t timestamp) {
    message_t msg;
    msg.push_back(toByte(Type::TIME));
    msg.push_back(sync);
    add_time(msg, timestamp);
    return msg;
  }

  void logError(const message_t& msg) {
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

  std::pair<sync_level_t, timestamp_t> parseTimeSyncMessage(const message_t& msg,
                                                                    Type expected) {
    const size_t expectedSize = sizeof(message_type_t) +
                                sizeof(sync_level_t) + sizeof(timestamp_t);

    if (msg.size() != expectedSize || type(msg) != expected) {
      logError(msg);
      throw std::runtime_error("Invalid SYNC_START message");
    }
    
    sync_level_t lvl = msg[1];

    timestamp_t t;
    std::memcpy(&t, &msg[2], sizeof(t));
    t = be64toh(t);

    return {lvl, t};
  }
} // Message namespace end