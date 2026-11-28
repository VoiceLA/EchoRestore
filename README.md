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
shared WAV tooling. 
