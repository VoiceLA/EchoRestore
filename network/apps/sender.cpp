#include "cli.hpp"
#include "echorestore/network/pipeline.hpp"

int main(int argc, char **argv) {
  return cli::run(
      argc, argv,
      "Usage: echorestore_sender --input FILE.wav [--session-id N]\n"
      "  [--write-manifest FILE.session] [--host IPv4 --port N] [--pace]\n"
      "Without --host, writes metadata only. Fixed 20 ms frames. Outputs are "
      "overwritten.",
      [&] {
        const cli::Arguments args(argc, argv,
                                  " --input  --session-id  --write-manifest  "
                                  "--host  --port  --pace ");

        echorestore::network::SenderOptions options;

        options.input = args.require("--input");
        options.manifest = args.get("--write-manifest");
        options.host = args.get("--host");
        options.session_id = args.number("--session-id", 1);
        options.pace = !args.get("--pace").empty();

        if (!options.host.empty())
          options.port = args.port("--port");
        else if (!args.get("--port").empty())
          throw std::invalid_argument("--port requires --host");

        if (!options.manifest.empty() &&
            std::filesystem::absolute(options.input).lexically_normal() ==
                std::filesystem::absolute(options.manifest).lexically_normal())
          throw std::invalid_argument(
              "Input and manifest must have distinct paths");

        echorestore::network::send(options);
      });
}
