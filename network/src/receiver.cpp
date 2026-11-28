#include "echorestore/audio/wav.hpp"
#include "echorestore/network/pipeline.hpp"
#include "session.hpp"
#include "udp.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

namespace echorestore::network {

void receive(const ReceiverOptions &options) {
  const auto session = detail::read_manifest(options.manifest);

  if (options.timeout_ms == 0)
    throw std::invalid_argument("Timeout must be positive");

  const auto output_path =
      std::filesystem::absolute(options.output).lexically_normal();
  const auto report_path =
      std::filesystem::absolute(options.report).lexically_normal();
  const auto manifest_path =
      std::filesystem::absolute(options.manifest).lexically_normal();

  if (output_path == report_path || output_path == manifest_path ||
      report_path == manifest_path)
    throw std::invalid_argument(
        "Manifest, output and report must have distinct paths");

  detail::UdpSocket socket;
  socket.bind(detail::address(options.bind, options.port));
  const auto deadline =
      detail::Clock::now() + std::chrono::milliseconds{options.timeout_ms};

  audio::Pcm16Audio audio;
  audio.samples.resize(session.total_samples);

  const auto expected = detail::packet_count(session);

  // Frame availability is independent of PCM values; expands to sample ranges
  // in the report. Missing PCM slots are never used as known audio.
  std::vector<bool> available(expected, false);

  std::uint64_t received = 0, duplicates = 0, conflicts = 0, reordered = 0,
                malformed = 0;
  std::uint32_t highest = 0;
  std::array<std::uint8_t, protocol::max_datagram_bytes> buffer{};

  std::cout << "Receiver ready\n" << std::flush;

  while (received < expected && detail::Clock::now() < deadline) {
    if (!socket.readable(deadline))
      continue;

    if (detail::Clock::now() >= deadline)
      break;

    const auto size = socket.read(buffer);

    if (size < 0)
      continue;

    if (static_cast<std::size_t>(size) > buffer.size()) {
      ++malformed;
      continue;
    }

    protocol::AudioPacket packet;

    try {
      packet = protocol::decode_packet(
          std::span{buffer.data(), static_cast<std::size_t>(size)}, session);
    } catch (const std::invalid_argument &) {
      ++malformed;
      continue;
    }

    auto destination = audio.samples.begin() + packet.sample_offset;

    if (available[packet.sequence]) {
      ++duplicates;

      if (!std::equal(packet.samples.begin(), packet.samples.end(),
                      destination))
        ++conflicts;
      continue; // first copy wins, including on conflicting duplicates
    }

    if (received > 0 && packet.sequence < highest)
      ++reordered;

    highest = std::max(highest, packet.sequence);
    std::copy(packet.samples.begin(), packet.samples.end(), destination);
    available[packet.sequence] = true;
    ++received;
  }

  const char *reason = received == expected ? "all_received" : "timeout";
  std::ostringstream report;
  report << "{\n  \"version\": 1,\n  \"session_id\": " << session.session_id
         << ",\n  \"sample_rate\": 16000,\n  \"channels\": 1,\n  \"format\": "
            "\"PCM_S16LE\","
         << "\n  \"total_samples\": " << session.total_samples
         << ",\n  \"frame_samples\": 320,\n  \"expected_packet_count\": "
         << expected << ",\n  \"received_packet_count\": " << received
         << ",\n  \"missing_packet_count\": " << expected - received
         << ",\n  \"duplicate_count\": " << duplicates
         << ",\n  \"conflicting_duplicate_count\": " << conflicts
         << ",\n  \"out_of_order_count\": " << reordered
         << ",\n  \"malformed_count\": " << malformed
         << ",\n  \"termination_reason\": \"" << reason
         << "\",\n  \"missing_ranges\": [";

  bool first = true;

  for (std::uint32_t i = 0; i < expected;) {
    if (available[i]) {
      ++i;
      continue;
    }

    const auto start = std::uint64_t{i} * protocol::frame_samples;

    while (i < expected && !available[i])
      ++i;

    const auto end = std::min(std::uint64_t{i} * protocol::frame_samples,
                              std::uint64_t{session.total_samples});

    if (!first)
      report << ',';

    report << "\n    {\"start\": " << start << ", \"length\": " << end - start
           << '}';
    first = false;
  }

  report << "\n  ]\n}\n";

  // Only at serialization are unavailable positions exposed as zero-filled PCM.
  detail::prepare_parent(options.output);
  auto temporary = options.output;
  temporary += ".tmp";

  audio::write_wav(temporary, audio);
  std::filesystem::rename(temporary, options.output);

  detail::write_text(options.report, report.str());

  std::cout << "Received " << received << '/' << expected << "; " << reason
            << '\n';
}

} // namespace echorestore::network
