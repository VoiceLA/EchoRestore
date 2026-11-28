#include "echorestore/network/pipeline.hpp"
#include "udp.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <csignal>
#include <iostream>
#include <queue>
#include <random>
#include <vector>

namespace echorestore::network {
namespace {

volatile std::sig_atomic_t stopped = 0;

extern "C" void stop_emulator(int) { stopped = 1; }

class SignalHandlers {
public:
  SignalHandlers() {
    struct sigaction action {};
    action.sa_handler = stop_emulator;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, &old_int_) < 0)
      detail::socket_error("sigaction");

    if (sigaction(SIGTERM, &action, &old_term_) < 0) {
      sigaction(SIGINT, &old_int_, nullptr);
      detail::socket_error("sigaction");
    }
  }

  ~SignalHandlers() {
    sigaction(SIGINT, &old_int_, nullptr);
    sigaction(SIGTERM, &old_term_, nullptr);
  }

private:
  struct sigaction old_int_ {
  }, old_term_{};
};

struct Pending {
  detail::Clock::time_point due;
  std::uint64_t order;
  std::vector<std::uint8_t> bytes;
};

struct Later {
  bool operator()(const Pending &a, const Pending &b) const {
    return a.due != b.due ? a.due > b.due : a.order > b.order;
  }
};

} // namespace

void emulate(const EmulatorOptions &options) {
  if (!std::isfinite(options.loss) || options.loss < 0 || options.loss > 1 ||
      options.delay_ms > 60000 || options.jitter_ms > 60000)
    throw std::invalid_argument(
        "Loss must be in [0,1]; delay/jitter in 0..60000 ms");

  const auto destination =
      detail::address(options.forward_host, options.forward_port);
  detail::UdpSocket socket;
  socket.bind(detail::address(options.bind, options.listen_port));

  const auto local = detail::address(options.bind, options.listen_port);

  if (local.sin_port == destination.sin_port &&
      (local.sin_addr.s_addr == destination.sin_addr.s_addr ||
       local.sin_addr.s_addr == INADDR_ANY))
    throw std::invalid_argument(
        "Emulator must not forward to its own listening endpoint");

  stopped = 0;
  SignalHandlers signals;

  std::mt19937 random(options.seed);

  // Separate generators keep the drop decisions unchanged when delay/jitter
  // changes. Raw mt19937 outputs give portable seeded decisions.
  std::mt19937 timing(options.seed);
  auto unit = [](std::mt19937 &generator) {
    return static_cast<double>(generator()) / 4294967296.0;
  };

  std::priority_queue<Pending, std::vector<Pending>, Later> queue;
  constexpr std::size_t max_queue_bytes = 16 * 1024 * 1024;
  std::size_t queued_bytes = 0;
  std::array<std::uint8_t, 65536> buffer{};

  std::uint64_t received = 0, dropped = 0, forwarded = 0, failed = 0,
                overflow = 0;

  std::cout << "Emulator ready; loss=" << options.loss
            << " delay_ms=" << options.delay_ms
            << " jitter_ms=" << options.jitter_ms << " seed=" << options.seed
            << '\n'
            << std::flush;

  while (!stopped) {
    while (!stopped && !queue.empty() &&
           queue.top().due <= detail::Clock::now()) {
      const auto &packet = queue.top();

      if (socket.write(packet.bytes, destination))
        ++forwarded;
      else
        ++failed;

      queued_bytes -= packet.bytes.size();
      queue.pop();
    }

    const auto next =
        queue.empty() ? detail::Clock::now() + std::chrono::milliseconds{100}
                      : queue.top().due;

    if (!socket.readable(next) || stopped)
      continue;

    const auto size = socket.read(buffer);

    if (size < 0)
      continue;

    ++received;

    if (unit(random) < options.loss) {
      ++dropped;
      continue;
    }

    const auto delay = std::max(0.0, options.delay_ms + (2 * unit(timing) - 1) *
                                                            options.jitter_ms);
    const auto bytes = std::span{buffer.data(), static_cast<std::size_t>(size)};

    if (delay == 0) {
      if (socket.write(bytes, destination))
        ++forwarded;
      else
        ++failed;
    } else if (queue.size() >= 65536 ||
               bytes.size() > max_queue_bytes - queued_bytes)
      ++overflow;
    else {
      const auto due = detail::Clock::now() +
                       std::chrono::duration_cast<detail::Clock::duration>(
                           std::chrono::duration<double, std::milli>{delay});

      queue.push(Pending{due, received, {bytes.begin(), bytes.end()}});
      queued_bytes += bytes.size();
    }
  }

  std::cout << "received=" << received << " dropped=" << dropped
            << " forwarded=" << forwarded << " failed_to_forward=" << failed
            << " queue_overflow=" << overflow
            << " discarded_on_shutdown=" << queue.size() << '\n';
}

} // namespace echorestore::network
