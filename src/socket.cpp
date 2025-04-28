#include "socket.hpp"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <ifaddrs.h>


Socket::Socket(std::optional<std::string> ip, port_t port) {
  buffer.resize(MAX_DATAGRAM_SIZE);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    error("socket ", std::strerror(errno));
    exit(ERROR_EXIT_CODE);
  }

  // Set socket to nonblocking mode.
  if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
    error("socket ", std::strerror(errno));
    exit(ERROR_EXIT_CODE);
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
      exit(ERROR_EXIT_CODE);
    }

    sockaddr_in* resolved = reinterpret_cast<sockaddr_in*>(res->ai_addr);
    addr.sin_addr = resolved->sin_addr;

    freeaddrinfo(res);
  }

  if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    error("INVALID bind ", std::strerror(errno));
    exit(ERROR_EXIT_CODE);
  }
}

Socket::~Socket() {
  if (sockfd >= 0) {
    close(sockfd);
  }
}

void Socket::sendTo(message_t& message, const PeerID& peer) {
  if (STOP.load()) {
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
    errno = 0;
    throw std::runtime_error("sendto");
  }
}

std::optional<Socket::ReceivedMessage> Socket::recvFrom() {
  if (STOP.load()) {
    return std::nullopt;
  }

  sockaddr_in senderAddr{};
  socklen_t addrLen = sizeof(senderAddr);

  ssize_t received = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
                              reinterpret_cast<sockaddr*>(&senderAddr), &addrLen);

  if (received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      errno = 0;
      return std::nullopt; // Timeout occurred
    } else {
      error("recvfrom ", std::strerror(errno));
      exit(ERROR_EXIT_CODE);
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

peer_set_t Socket::getBoundAddressAndPort() {
  peer_set_t result;

  sockaddr_in localAddr {};
  socklen_t addrLen = sizeof(localAddr);

  if (getsockname(sockfd, reinterpret_cast<sockaddr*>(&localAddr), &addrLen) < 0) {
    error("getsockname: ", std::strerror(errno));
    throw std::runtime_error("Error retrieving bound address and port");
  }

  address_t ip = ntohl(localAddr.sin_addr.s_addr);
  port_t port = ntohs(localAddr.sin_port);

  if (localAddr.sin_addr.s_addr == INADDR_ANY) {
    // Bound to all interfaces.
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
      error("getifaddrs: ", std::strerror(errno));
      throw std::runtime_error("Error retrieving network interfaces");
    }

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr == nullptr) continue;
      if (ifa->ifa_addr->sa_family != AF_INET) continue; // Only IPv4

      sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
      address_t iface_ip = ntohl(addr->sin_addr.s_addr);
      
      result.emplace(iface_ip, port);
    }

    freeifaddrs(ifaddr);
  } else {
    // Bound to a specific interface
    result.emplace(ip, port);
  }

  return result;
}
