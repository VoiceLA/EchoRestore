#include "cli.hpp"
#include "echorestore/network/pipeline.hpp"

int main(int argc, char **argv) {
  return cli::run(
      argc, argv,
      "Usage: echorestore_receiver --bind IPv4 --port N --manifest "
      "FILE.session\n"
      "  --output FILE.wav --report FILE.json [--timeout-ms 10000]\n"
      "Timeout includes sender startup time. Outputs are overwritten.",
      [&] {
        const cli::Arguments args(
            argc, argv,
            " --bind  --port  --manifest  --output  --report  --timeout-ms ");

        echorestore::network::receive(
            {args.require("--bind"), args.port("--port"),
             args.require("--manifest"), args.require("--output"),
             args.require("--report"), args.number("--timeout-ms", 10000)});
      });
}
