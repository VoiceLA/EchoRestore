#include "echorestore/protocol/packet.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

namespace echorestore::protocol {
namespace {

constexpr std::array<std::uint8_t, 4> magic{'E', 'C', 'H', 'O'};

[[noreturn]] void fail(std::string_view message) {
  throw std::invalid_argument("Packet: " + std::string(message));
}

std::uint32_t read_be(const std::uint8_t *bytes, unsigned count) {
  std::uint32_t value = 0;

  for (unsigned i = 0; i < count; ++i) {
    value = (value << 8) | bytes[i];
  }

  return value;
}

void write_be(std::uint8_t *bytes, std::uint32_t value, unsigned count) {
  for (unsigned i = 0; i < count; ++i) {
    bytes[count - i - 1] = static_cast<std::uint8_t>(value & 0xffU);
    value >>= 8;
  }
}

void validate_frame(std::uint32_t session_id, std::uint32_t sequence,
                    std::uint32_t sample_offset, std::size_t sample_count,
                    const SessionBounds &session) {
  if (session_id != session.session_id)
    fail("session ID does not match expected session");

  if (sample_count == 0 || sample_count > frame_samples)
    fail("sample count must be between 1 and 320");

  const auto expected_offset = std::uint64_t{sequence} * frame_samples;

  if (expected_offset != sample_offset)
    fail("sample offset must equal sequence * 320 without overflow");

  if (sample_offset >= session.total_samples)
    fail("frame starts outside the audio bounds");

  const auto remaining = session.total_samples - sample_offset;
  const auto expected_count = std::min(frame_samples, remaining);

  if (sample_count != expected_count)
    fail("frame length does not match session bounds (only final frame may be "
         "short)");
}

} // namespace

std::vector<std::uint8_t> encode_packet(const AudioPacket &packet,
                                        const SessionBounds &session) {
  validate_frame(packet.session_id, packet.sequence, packet.sample_offset,
                 packet.samples.size(), session);

  const auto sample_count = static_cast<std::uint32_t>(packet.samples.size());
  std::vector<std::uint8_t> datagram(header_bytes + sample_count * 2, 0);

  std::copy(magic.begin(), magic.end(), datagram.begin());
  datagram[4] = 1; // version
  datagram[5] = 1; // audio

  // bytes 6..7 are reserved and remain zero
  write_be(datagram.data() + 8, packet.session_id, 4);
  write_be(datagram.data() + 12, packet.sequence, 4);
  write_be(datagram.data() + 16, packet.sample_offset, 4);
  write_be(datagram.data() + 20, sample_count, 2);
  write_be(datagram.data() + 22, sample_count * 2, 2);

  for (std::size_t i = 0; i < packet.samples.size(); ++i) {
    const auto bits = static_cast<std::uint16_t>(packet.samples[i]);
    datagram[header_bytes + i * 2] = static_cast<std::uint8_t>(bits & 0xffU);
    datagram[header_bytes + i * 2 + 1] = static_cast<std::uint8_t>(bits >> 8);
  }

  return datagram;
}

AudioPacket decode_packet(std::span<const std::uint8_t> datagram,
                          const SessionBounds &session) {
  if (datagram.size() < header_bytes || datagram.size() > max_datagram_bytes)
    fail("datagram length must be between 24 and 664 bytes");

  if (!std::equal(magic.begin(), magic.end(), datagram.begin()))
    fail("invalid ECHO magic");

  if (datagram[4] != 1 || datagram[5] != 1)
    fail("unsupported version or packet type");

  if (read_be(datagram.data() + 6, 2) != 0)
    fail("reserved header fields must be zero");

  const auto session_id = read_be(datagram.data() + 8, 4);
  const auto sequence = read_be(datagram.data() + 12, 4);
  const auto sample_offset = read_be(datagram.data() + 16, 4);
  const auto sample_count = read_be(datagram.data() + 20, 2);
  const auto payload_bytes = read_be(datagram.data() + 22, 2);

  if (payload_bytes != sample_count * 2 ||
      datagram.size() != header_bytes + payload_bytes)
    fail("payload size must match sample count and exact datagram length");

  validate_frame(session_id, sequence, sample_offset, sample_count, session);

  AudioPacket packet{session_id, sequence, sample_offset, {}};
  packet.samples.resize(sample_count);

  for (std::size_t i = 0; i < packet.samples.size(); ++i) {
    const auto *bytes = datagram.data() + header_bytes + i * 2;
    const auto bits = std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8);
    const auto value = bits < 0x8000U ? static_cast<std::int32_t>(bits)
                                      : static_cast<std::int32_t>(bits) - 65536;
    packet.samples[i] = static_cast<std::int16_t>(value);
  }

  return packet;
}

} // namespace echorestore::protocol
