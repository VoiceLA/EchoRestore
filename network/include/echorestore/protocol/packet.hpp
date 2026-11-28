#ifndef NETWORK_INCLUDE_ECHORESTORE_PROTOCOL_PACKET_HPP
#define NETWORK_INCLUDE_ECHORESTORE_PROTOCOL_PACKET_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace echorestore::protocol {

inline constexpr std::size_t header_bytes = 24;
inline constexpr std::uint32_t frame_samples = 320;
inline constexpr std::size_t max_datagram_bytes =
    header_bytes + frame_samples * 2;

// Known out-of-band metadata, not an on-wire control message or manifest
// parser. Validate wider source lengths before narrowing to total_samples.
struct SessionBounds {
  std::uint32_t session_id;
  std::uint32_t total_samples;
};

// In-memory values only: this struct must never be sent as raw bytes.
// sample_count and payload_bytes are derived from samples when encoding.
struct AudioPacket {
  std::uint32_t session_id;
  std::uint32_t sequence;
  std::uint32_t sample_offset;

  std::vector<std::int16_t> samples;
};

// Protocol v1: big-endian header, little-endian signed PCM16 payload.
// Both functions validate session, frame position/length and audio boundaries.
// Empty sessions contain no valid audio packets. Invalid inputs throw
// std::invalid_argument. Allocation failures propagate normally.
[[nodiscard]] std::vector<std::uint8_t>
encode_packet(const AudioPacket &packet, const SessionBounds &session);

// Pass the entire datagram, never a truncated receive buffer. Oversized input
// is rejected before allocation. Decoded samples own their storage.
[[nodiscard]] AudioPacket decode_packet(std::span<const std::uint8_t> datagram,
                                        const SessionBounds &session);

} // namespace echorestore::protocol

#endif // NETWORK_INCLUDE_ECHORESTORE_PROTOCOL_PACKET_HPP
