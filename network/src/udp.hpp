#ifndef NETWORK_SRC_UDP_HPP
#define NETWORK_SRC_UDP_HPP

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>

namespace echorestore::network::detail {

using Clock = std::chrono::steady_clock;

[[noreturn]] inline void socket_error(const char *operation) {
  throw std::system_error(errno, std::generic_category(), operation);
}

inline sockaddr_in address(const std::string &host, std::uint16_t port) {
  sockaddr_in result{};
  result.sin_family = AF_INET;
  result.sin_port = htons(port);

  if (port == 0 || inet_pton(AF_INET, host.c_str(), &result.sin_addr) != 1)
    throw std::invalid_argument(
        "Expected numeric IPv4 address and port in 1..65535");

  return result;
}

class UdpSocket {
public:
  UdpSocket()
      : fd_(::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) {
    if (fd_ < 0)
      socket_error("socket");
  }

  ~UdpSocket() { ::close(fd_); }
  UdpSocket(const UdpSocket &) = delete;
  UdpSocket &operator=(const UdpSocket &) = delete;

  void bind(const sockaddr_in &local) const {
    if (::bind(fd_, reinterpret_cast<const sockaddr *>(&local), sizeof(local)) <
        0)
      socket_error("bind");
  }

  bool readable(Clock::time_point deadline) const {
    const auto remaining = deadline - Clock::now();

    if (remaining <= Clock::duration::zero())

      return false;

    // Round up to avoid spinning for a sub-millisecond remainder.
    auto ms = std::chrono::ceil<std::chrono::milliseconds>(remaining).count();

    if (ms > 100)
      ms = 100; // also bounds signal/shutdown latency

    pollfd descriptor{fd_, POLLIN, 0};
    const auto result = ::poll(&descriptor, 1, static_cast<int>(ms));

    if (result < 0) {
      if (errno == EINTR)
        return false;

      socket_error("poll");
    }

    if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
      throw std::runtime_error("UDP socket poll error");

    return result > 0 && (descriptor.revents & POLLIN);
  }

  // MSG_TRUNC returns the full datagram length on Linux. Callers must reject
  // lengths greater than the buffer, rather than processing a valid prefix.
  ssize_t read(std::span<std::uint8_t> buffer) const {
    const auto size = ::recvfrom(fd_, buffer.data(), buffer.size(), MSG_TRUNC,
                                 nullptr, nullptr);

    if (size < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
      socket_error("recvfrom");

    return size;
  }

  bool write(std::span<const std::uint8_t> bytes,
             const sockaddr_in &destination) const {
    ssize_t size;
    do {
      size = ::sendto(fd_, bytes.data(), bytes.size(), 0,
                      reinterpret_cast<const sockaddr *>(&destination),
                      sizeof(destination));
    } while (size < 0 && errno == EINTR);

    return size >= 0 && static_cast<std::size_t>(size) == bytes.size();
  }

private:
  int fd_;
};

} // namespace echorestore::network::detail

#endif // NETWORK_SRC_UDP_HPP
