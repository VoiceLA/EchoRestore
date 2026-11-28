# Experiments

The current implementation supports synthetic WAV generation and
[WAV I/O](../../common/docs/WAV.md). It does not transmit datagrams or produce damaged audio or
loss reports.

See [examples](../../examples/README.md) for fixture generation and
[milestones](../../docs/MILESTONES.md) for the implementation sequence.

Later verification will cover: loss-free sample equality; 100% configured drop;
seeded 5% dropping; out-of-order packets; duplicates/conflicts; malformed packets;
missing final frame; 16,001-sample input; clean process startup and shutdown.
Integration tests will use real loopback UDP and robust process orchestration.

Configured emulator drop probability and observed missing-by-cutoff fraction
are distinct measurements. A fixed seed does not imply exactly 5% observed loss.
The future receiver timeout includes waiting for the sender to start; start the
receiver first, then emulator, then sender within that overall deadline. Stop
the emulator by controlled signal, not by assuming that it knows a file ended.
