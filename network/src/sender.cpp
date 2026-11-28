#include "echorestore/audio/wav.hpp"
#include "echorestore/network/pipeline.hpp"
#include "session.hpp"
#include "udp.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <thread>

namespace echorestore::network {

void send(const SenderOptions &options) {
  const auto audio = audio::read_wav(options.input);

  if (audio.samples.size() > std::numeric_limits<std::uint32_t>::max())
    throw std::runtime_error("Source exceeds protocol 32-bit sample limit");

  const protocol::SessionBounds session{
      options.session_id, static_cast<std::uint32_t>(audio.samples.size())};

  if (!options.manifest.empty())
    detail::write_text(options.manifest, detail::manifest_text(session));

  if (options.host.empty()) {
    if (options.manifest.empty())
      throw std::invalid_argument("Specify destination or --write-manifest");

    std::cout << "Manifest written: " << detail::packet_count(session)
              << " expected packets\n";

    return;
  }

  const auto destination = detail::address(options.host, options.port);
  detail::UdpSocket socket;

  const auto start = detail::Clock::now();

  for (std::uint32_t seq = 0; seq < detail::packet_count(session); ++seq) {
    const auto offset = seq * protocol::frame_samples;
    const auto count =
        std::min(protocol::frame_samples, session.total_samples - offset);

    protocol::AudioPacket packet{session.session_id,
                                 seq,
                                 offset,
                                 {audio.samples.begin() + offset,
                                  audio.samples.begin() + offset + count}};

    const auto bytes = protocol::encode_packet(packet, session);

    if (options.pace)
      std::this_thread::sleep_until(start +
                                    std::chrono::milliseconds{20LL * seq});

    if (!socket.write(bytes, destination))
      detail::socket_error("sendto");
  }

  std::cout << "Sent " << detail::packet_count(session) << " packets\n";
}

} // namespace echorestore::network
