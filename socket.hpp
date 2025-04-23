#pragma once

#include <string>
#include <optional>
#include <sys/socket.h>
#include "common.hpp"


class Socket {
 public:
  struct ReceivedMessage {
    PeerID from;
    message_t data;
    TimePoint receivedAt;
  };
  Socket(std::optional<std::string> ip = std::nullopt, uint16_t port = 0);
  ~Socket();

  void setReadTimeOut(std::chrono::seconds sec);
  void sendTo(message_t& message, const PeerID& peer);
  std::optional<ReceivedMessage> recvFrom();

  // Both address and port are returned in host byte order.
  std::pair<peer_ip_t, peer_port_t> getBoundAddressAndPort();


 private:
  int sockfd;
  message_t buffer;
};
