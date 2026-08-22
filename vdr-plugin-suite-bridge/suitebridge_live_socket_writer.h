#ifndef VDR_SUITE_BRIDGE_LIVE_SOCKET_WRITER_H
#define VDR_SUITE_BRIDGE_LIVE_SOCKET_WRITER_H

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <poll.h>
#include <sys/socket.h>

inline bool SuiteBridgeLiveSendAll(
    int fd,
    const std::uint8_t *data,
    std::size_t size,
    const std::atomic<bool> &stopping,
    const std::atomic<bool> &terminal)
{
  if (fd < 0 || (data == nullptr && size != 0)) return false;

  std::size_t offset = 0;
  while (offset < size) {
    if (stopping.load(std::memory_order_acquire) ||
        terminal.load(std::memory_order_acquire)) {
      return false;
    }

    pollfd descriptor {};
    descriptor.fd = fd;
    descriptor.events = POLLOUT;
    const int polled = ::poll(&descriptor, 1, 250);
    if (polled < 0) {
      if (errno == EINTR) continue;
      return false;
    }
    if (polled == 0) {
      // POLLOUT backpressure is not a disconnect. The receiver-side bounded
      // TS buffer remains the hard fail-closed limit; retry here so a short
      // consumer pause cannot tear down an otherwise healthy Live source.
      continue;
    }
    if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      return false;
    }
    if ((descriptor.revents & POLLOUT) == 0) continue;

    const ssize_t written = ::send(
        fd,
        data + offset,
        size - offset,
#ifdef MSG_NOSIGNAL
        MSG_NOSIGNAL
#else
        0
#endif
    );
    if (written > 0) {
      offset += static_cast<std::size_t>(written);
      continue;
    }
    if (written < 0 &&
        (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    }
    return false;
  }
  return true;
}

#endif
