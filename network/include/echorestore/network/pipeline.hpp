#ifndef NETWORK_INCLUDE_ECHORESTORE_NETWORK_PIPELINE_HPP
#define NETWORK_INCLUDE_ECHORESTORE_NETWORK_PIPELINE_HPP

#include <cstdint>
#include <filesystem>
#include <string>

namespace echorestore::network {

struct SenderOptions {
  std::filesystem::path input;

  std::filesystem::path manifest;

  std::string host;
  std::uint16_t port = 0;

  std::uint32_t session_id = 1;
  bool pace = false;
};

struct ReceiverOptions {
  std::string bind;
  std::uint16_t port;

  std::filesystem::path manifest;
  std::filesystem::path output;
  std::filesystem::path report;

  std::uint32_t timeout_ms = 10000;
};

struct EmulatorOptions {
  std::string bind;
  std::uint16_t listen_port;

  std::string forward_host;
  std::uint16_t forward_port;

  double loss = 0;
  std::uint32_t delay_ms = 0;
  std::uint32_t jitter_ms = 0;
  std::uint32_t seed = 42;
};

// With an empty host, only generate the manifest, without sending audio.
void send(const SenderOptions &options);

void receive(const ReceiverOptions &options);

// Runs until SIGINT/SIGTERM; queued datagrams are discarded on shutdown.
void emulate(const EmulatorOptions &options);

} // namespace echorestore::network

#endif // NETWORK_INCLUDE_ECHORESTORE_NETWORK_PIPELINE_HPP
