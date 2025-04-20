#include "socket.hpp"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>


Socket::Socket(std::optional<std::string> ip, uint16_t port) {
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    error("socket ", std::strerror(errno));
    exit(1);
  }

  sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (!ip.has_value()) {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    if (inet_pton(AF_INET, ip->c_str(), &addr.sin_addr) < 0) {
      error("INVALID IP address ", ip.value(), std::strerror(errno));
      exit(1);
    }
  }

  if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    error("INVALID bind ", std::strerror(errno));
    exit(1);
  }
}

Socket::~Socket() {
  if (sockfd >= 0) {
    close(sockfd);
  }
}

void Socket::sendTo(message_t& message, const PeerID& peer) {
  sockaddr_in destAddr{};
  destAddr.sin_family = AF_INET;
  destAddr.sin_addr.s_addr = htonl(peer.ip);
  destAddr.sin_port = htons(peer.port);

  ssize_t sent = sendto(sockfd, message.data(), message.size(), 0,
                        reinterpret_cast<const sockaddr*>(&destAddr), sizeof(destAddr));
  if (sent < 0) {
    error("sendto ", std::strerror(errno));
    exit(1);
  }
}

std::optional<Socket::ReceivedMessage> Socket::recvFrom() {
  message_t buffer;
  buffer.resize(std::numeric_limits<uint16_t>::max());

  sockaddr_in senderAddr{};
  socklen_t addrLen = sizeof(senderAddr);

  ssize_t received = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
                              reinterpret_cast<sockaddr*>(&senderAddr), &addrLen);

  if (received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return std::nullopt; // Timeout occurred
    } else {
      error("recvfrom ", std::strerror(errno));
      exit(1);
    }
  }

  buffer.resize(received);
  PeerID sender;
  sender.ip = ntohl(senderAddr.sin_addr.s_addr);
  sender.port = ntohs(senderAddr.sin_port);

  return ReceivedMessage{
    .from = sender,
    .data = std::move(buffer),
    .receivedAt = Clock::now()
  };
}

void Socket::setReadTimeOut(std::chrono::seconds sec) {
  struct timeval timeout;
  timeout.tv_sec = sec.count();
  timeout.tv_usec = 0;

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    error("setsockopt ", std::strerror(errno));
    throw std::runtime_error("Error");
  }
}
