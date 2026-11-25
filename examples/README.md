# Deterministic fixture

From the repository root, after building:

```bash
./build/echorestore_generate_sample examples/sample.wav
```

The helper creates parent directories and overwrites its output file. It writes
one second (16,000 samples) of 400 Hz square wave with amplitude ±8000:
16 kHz, mono, signed PCM16 little-endian in a 32,044-byte RIFF/WAVE file.
Integer arithmetic makes the fixture byte-for-byte reproducible without a seed,
Python, external assets or audio dependencies. It is synthetic test data, not a
quality listening sample. Generated WAVs are ignored by Git.

The generator uses the shared [WAV library](../common/docs/WAV.md) to write the fixture.
No session manifest is generated yet. In M2 the canonical workflow will
be `echorestore_sender --input examples/sample.wav --write-manifest examples/sample.json`:
metadata will be derived from the WAV, without manually entering `total_samples`.
That command is planned and is not supported by the current sender stub.
