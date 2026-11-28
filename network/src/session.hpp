#ifndef NETWORK_SRC_SESSION_HPP
#define NETWORK_SRC_SESSION_HPP

#include "echorestore/protocol/packet.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace echorestore::network::detail {

inline std::uint32_t unsigned_value(std::string_view value) {
  std::uint32_t result = 0;

  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), result);

  if (error != std::errc{} || end != value.data() + value.size() ||
      value.empty())
    throw std::invalid_argument("Expected unsigned 32-bit integer: " +
                                std::string(value));

  return result;
}

inline std::uint32_t packet_count(const protocol::SessionBounds &session) {
  return static_cast<std::uint32_t>(
      (std::uint64_t{session.total_samples} + 319) / 320);
}

inline void prepare_parent(const std::filesystem::path &path) {
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path());
}

// Each output is replaced atomically on the same filesystem. The WAV/report
// pair is not a transaction. A failed write leaves the previous file intact.
inline void write_text(const std::filesystem::path &path,
                       const std::string &text) {
  prepare_parent(path);

  auto temporary = path;
  temporary += ".tmp";

  std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
  out << text;
  out.close();

  if (!out)
    throw std::runtime_error("Cannot write " + temporary.string());

  std::filesystem::rename(temporary, path);
}

inline std::string manifest_text(const protocol::SessionBounds &session) {
  std::ostringstream out;
  out << "ECHORESTORE_SESSION 1\n"
      << "session_id " << session.session_id << '\n'
      << "sample_rate 16000\nchannels 1\nformat PCM_S16LE\n"
      << "total_samples " << session.total_samples << '\n'
      << "frame_samples 320\nexpected_packet_count " << packet_count(session)
      << '\n';

  return out.str();
}

inline protocol::SessionBounds
read_manifest(const std::filesystem::path &path) {
  std::ifstream in(path);

  if (!in)
    throw std::runtime_error("Cannot open manifest: " + path.string());

  auto field = [&](std::string_view expected) {
    std::string key, value;

    if (!(in >> key >> value) || key != expected || value.size() > 32)
      throw std::runtime_error("Invalid manifest field: " +
                               std::string(expected));

    return value;
  };

  if (field("ECHORESTORE_SESSION") != "1")
    throw std::runtime_error("Unsupported manifest version");

  const auto id = unsigned_value(field("session_id"));

  if (field("sample_rate") != "16000" || field("channels") != "1" ||
      field("format") != "PCM_S16LE")
    throw std::runtime_error("Unsupported manifest audio format");

  const auto total = unsigned_value(field("total_samples"));

  if (total > (std::numeric_limits<std::uint32_t>::max() - 36U) / 2U)
    throw std::runtime_error("Session exceeds PCM16 RIFF output size limit");

  const protocol::SessionBounds session{id, total};

  if (field("frame_samples") != "320" ||
      unsigned_value(field("expected_packet_count")) != packet_count(session))
    throw std::runtime_error("Inconsistent manifest frame/packet count");

  std::string extra;

  if (in >> extra)
    throw std::runtime_error("Unexpected trailing manifest content");

  return session;
}

} // namespace echorestore::network::detail

#endif // NETWORK_SRC_SESSION_HPP
