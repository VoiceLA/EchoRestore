#ifndef NETWORK_APPS_CLI_HPP
#define NETWORK_APPS_CLI_HPP

#include <charconv>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace cli {

class Arguments {
public:
  Arguments(int argc, char **argv, std::string_view allowed) {
    for (int i = 1; i < argc; ++i) {
      std::string key = argv[i];

      if (allowed.find(" " + key + " ") == std::string_view::npos ||
          values_.contains(key))
        throw std::invalid_argument("Unknown or repeated option: " + key);

      if (key == "--pace")
        values_[key] = "true";
      else {
        if (++i == argc || std::string_view{argv[i]}.starts_with("--"))
          throw std::invalid_argument("Missing value for " + key);

        values_[key] = argv[i];
      }
    }
  }

  std::string get(const std::string &key,
                  const std::string &fallback = "") const {
    const auto it = values_.find(key);

    return it == values_.end() ? fallback : it->second;
  }

  std::string require(const std::string &key) const {
    auto value = get(key);

    if (value.empty())
      throw std::invalid_argument("Required: " + key);

    return value;
  }

  std::uint32_t number(const std::string &key, std::uint32_t fallback) const {
    const auto value = get(key, std::to_string(fallback));
    std::uint32_t result = 0;

    const auto [end, error] =
        std::from_chars(value.data(), value.data() + value.size(), result);

    if (error != std::errc{} || end != value.data() + value.size())
      throw std::invalid_argument("Invalid integer for " + key);

    return result;
  }

  std::uint16_t port(const std::string &key) const {
    const auto value = number(key, 0);

    if (value == 0 || value > 65535)
      throw std::invalid_argument("Port must be in 1..65535: " + key);

    return static_cast<std::uint16_t>(value);
  }

  double loss() const {
    const auto value = get("--loss", "0");
    double result = 0;

    const auto [end, error] =
        std::from_chars(value.data(), value.data() + value.size(), result);

    if (error != std::errc{} || end != value.data() + value.size())
      throw std::invalid_argument("Invalid --loss; use a fraction, e.g. 0.05");

    return result;
  }

private:
  std::map<std::string, std::string> values_;
};

template <class Action>
int run(int argc, char **argv, const char *help, Action action) {
  if (argc == 2 && std::string_view{argv[1]} == "--help") {
    std::cout << help << '\n';

    return 0;
  }

  try {
    action();

    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n' << help << '\n';

    return 1;
  }
}

} // namespace cli

#endif // NETWORK_APPS_CLI_HPP
