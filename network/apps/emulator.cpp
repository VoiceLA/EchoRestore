#include "cli.hpp"
#include "echorestore/network/pipeline.hpp"

int main(int argc, char **argv) {
  return cli::run(
      argc, argv,
      "Usage: echorestore_emulator --bind IPv4 --listen-port N\n"
      "  --forward-host IPv4 --forward-port N [--loss 0.05]\n"
      "  [--delay-ms 50] [--jitter-ms 10] [--seed 42]\n"
      "Delay = max(0, delay + uniform(-jitter, +jitter)) ms. Stop: Ctrl+C.",
      [&] {
        const cli::Arguments args(
            argc, argv,
            " --bind  --listen-port  --forward-host  --forward-port  --loss  "
            "--delay-ms  --jitter-ms  --seed ");

        echorestore::network::emulate(
            {args.require("--bind"), args.port("--listen-port"),
             args.require("--forward-host"), args.port("--forward-port"),
             args.loss(), args.number("--delay-ms", 0),
             args.number("--jitter-ms", 0), args.number("--seed", 42)});
      });
}
