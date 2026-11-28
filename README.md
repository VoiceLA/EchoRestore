# EchoRestore

A small C++20 pipeline: WAV → UDP sender → network emulator → UDP receiver →
damaged WAV and a JSON report of missing samples. See [Русский](README.ru.md).

The current goal is a simple working network component with configurable packet
loss, delay and jitter. Numerical recovery is separate and is not implemented here.

## Build

Linux (including WSL2), CMake 3.20+ and GCC/Clang with C++20 are required.
There are no third-party dependencies.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/echorestore_generate_sample examples/sample.wav
./build/echorestore_sender --input examples/sample.wav --write-manifest examples/sample.session
```

Then start receiver, emulator and sender in three terminals, in that order:

```bash
./build/echorestore_receiver --bind 127.0.0.1 --port 9002 --manifest examples/sample.session --timeout-ms 10000 --output output/damaged.wav --report output/report.json
```

```bash
./build/echorestore_emulator --bind 127.0.0.1 --listen-port 9000 --forward-host 127.0.0.1 --forward-port 9002 --loss 0.05 --delay-ms 50 --jitter-ms 10 --seed 42
```

```bash
./build/echorestore_sender --input examples/sample.wav --host 127.0.0.1 --port 9000 --pace
```

`loss=0.05` drops each packet with probability 5%. Surviving packets have delay
`max(0, 50 + uniform(-10, 10))` ms. All three values default to zero.
Stop the emulator with Ctrl+C. The receiver's timeout starts when it binds,
so allow time for startup, file duration and maximum delay.
Outputs are overwritten; use distinct paths and one process per output path.

Only 16 kHz mono signed PCM16 WAV is supported; frames contain 320 samples
(20 ms), except the last frame. The original sample positions and total length
are preserved; missing samples become silence in the WAV and remain identifiable
through `missing_ranges` in the JSON report. The manifest contains metadata only.
The custom protocol is for trusted local/test networks and is not RTP.

## Code and documentation

- `common/`: [WAV I/O](common/docs/WAV.md) and sample generator; `echorestore::audio`.
- `network/`: codec (`echorestore::protocol`), transport and emulator (`echorestore::network`), three small CLI apps.
- [Usage and parameter details](network/docs/USAGE.md), [protocol](network/docs/PROTOCOL.md), [manual checks](network/docs/EXPERIMENTS.md).

Use `--help` on each executable. `-DECHORESTORE_BUILD_NETWORK=OFF` builds only
shared WAV tooling. Test infrastructure remains deferred to the team.

## Formatting

C/C++ sources use the LLVM style with **clang-format 18**. The formatter is
a development tool; it is not required to run the pipeline.

The normal workflow is `git push` → wait for GitHub Actions to format the sources
and push a formatting commit → `git pull`. No local Git hooks are used.

Optional manual checks and fixes for C/C++ files tracked by Git, from the
repository root:

```bash
sudo apt-get install clang-format-18

# Check formatting
git ls-files -z -- '*.cpp' '*.h' '*.hpp' | xargs -0 -r clang-format-18 --dry-run --Werror

# Apply formatting
git ls-files -z -- '*.cpp' '*.h' '*.hpp' | xargs -0 -r clang-format-18 -i
```

GitHub Actions checks every branch push and pull request. On push it fixes style,
rechecks and pushes a separate formatting commit; run `git pull` afterwards.
The repository must permit Actions to write contents and push to that branch.
Branch protection can reject the automatic push; in that case apply fixes locally.
Pull request runs show the formatting diff and fail until the sources are formatted,
including for forks. The workflow does not build or run the audio pipeline.
