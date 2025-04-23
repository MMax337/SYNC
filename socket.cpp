#include "socket.hpp"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>


Socket::Socket(std::optional<std::string> ip, uint16_t port) {
  buffer.resize(std::numeric_limits<uint16_t>::max());
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    error("socket ", std::strerror(errno));
    exit(1);
  }

  // Set socket to nonblocking mode.
  if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
    error("socket ", std::strerror(errno));
    exit(1);
  }

  sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (!ip.has_value()) {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    addrinfo hints {};
    hints.ai_family = AF_INET;   
    hints.ai_socktype = SOCK_DGRAM; 
    hints.ai_flags = AI_NUMERICSERV;

    addrinfo* res;
    int err = getaddrinfo(ip->c_str(), nullptr, &hints, &res);
    if (err != 0) {
      error("getaddrinfo failed for ", ip.value(), ": ", gai_strerror(err));
      freeaddrinfo(res);
      exit(1);
    }

    sockaddr_in* resolved = reinterpret_cast<sockaddr_in*>(res->ai_addr);
    addr.sin_addr = resolved->sin_addr;

    freeaddrinfo(res);
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
  if (stop_requested.load()) {
    return;
  }

  sockaddr_in destAddr{};
  destAddr.sin_family = AF_INET;
  destAddr.sin_addr.s_addr = htonl(peer.ip);
  destAddr.sin_port = htons(peer.port);

  ssize_t sent = sendto(sockfd, message.data(), message.size(), 0,
                        reinterpret_cast<const sockaddr*>(&destAddr), sizeof(destAddr));
  if (sent < 0) {
    if (errno == EINTR) return;
    error("sendto ", std::strerror(errno));
    throw std::runtime_error("sendto");
  }
}

std::optional<Socket::ReceivedMessage> Socket::recvFrom() {
  if (stop_requested.load()) {
    return std::nullopt;
  }

  sockaddr_in senderAddr{};
  socklen_t addrLen = sizeof(senderAddr);

  ssize_t received = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
                              reinterpret_cast<sockaddr*>(&senderAddr), &addrLen);

  if (received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return std::nullopt; // Timeout occurred
    } else {
      error("recvfrom ", std::strerror(errno));
      exit(1);
    }
  }

  message_t data(buffer.begin(), buffer.begin() + received);

  PeerID sender;
  sender.ip = ntohl(senderAddr.sin_addr.s_addr);
  sender.port = ntohs(senderAddr.sin_port);

  return ReceivedMessage{
    .from = sender,
    .data = std::move(data),
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
