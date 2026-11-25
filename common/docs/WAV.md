# WAV I/O (M1, audio portion)

`echorestore::audio` exposes `Pcm16Audio`, `read_wav(path)` and
`write_wav(path, audio)` through `echorestore/audio/wav.hpp`.
Link the CMake target `echorestore::audio`.

```cpp
#include <echorestore/audio/wav.hpp>

auto audio = echorestore::audio::read_wav("input.wav");
echorestore::audio::write_wav("output/copy.wav", audio);
```

The supported format remains RIFF/WAVE format tag 1, 16,000 Hz, one channel,
signed 16-bit PCM. `Pcm16Audio::samples` holds native `int16_t` values in
[-32768, 32767], without normalization. Sample rate and channels are explicit
fields, validated on writing. On disk the samples and numeric fields are
little-endian, independently of host byte order. Zero represents actual silence;
the future receiver must keep availability separately.

## Reader policies

- Parse chunks within the declared RIFF boundary, without fixed `fmt ` or `data`
  offsets. Skip unknown chunks and their odd-length padding byte. Padding need
  not be zero. Accept `data` before `fmt ` by deferring sample loading.
- Require exactly one `fmt ` and one `data` chunk. Reject duplicates as ambiguous.
- Accept the standard 16-byte PCM format chunk and 18-byte PCM WAVEFORMATEX with
  zero extension size. Other format extensions, including WAVE_FORMAT_EXTENSIBLE,
  compressed audio, RF64 and RIFX, are unsupported.
- Validate byte rate, block alignment, complete PCM16 samples, chunk boundaries,
  RIFF/file size agreement and buffer size before loading audio. Reject trailing
  bytes outside the declared RIFF container rather than silently ignoring them.
- An empty `data` chunk is valid and yields an empty sample vector.

The file is loaded into memory, not streamed. RIFF uses 32-bit chunk sizes;
this also bounds sample counts below the MVP's 32-bit offset limit. Reading a
very large valid file can still fail due to insufficient memory.

## Writer policies

Write a canonical 44-byte WAV header followed by the samples. Metadata and
unknown input chunks are not retained; PCM values are preserved exactly.
Validate format and the RIFF size limit before creating directories or opening
the destination. Create parent directories and overwrite an existing destination.
The write is not atomic: an I/O failure can leave a partial file. The application
layer may stage outputs before publishing them in the receiver milestone.

Invalid/unsupported WAV data and stream I/O failures raise `std::runtime_error`;
directory errors raise `std::filesystem::filesystem_error`. Allocation failures
propagate normally. Callers should include the input/output path in their error
context. No third-party dependency, UDP, resampling or interpolation is involved.

The team will add automated tests separately. The sample generator now uses
this library's writer instead of maintaining a separate RIFF implementation.

## Manual verification of this increment

The Debug build completed with GCC 14.2 and `-Wall -Wextra -Wpedantic` without
warnings. A temporary driver outside the repository performed read/write
roundtrips, with the resulting PCM checked independently using Python's standard
`wave` module (Python is not a build/runtime dependency).

Verified a 16,000-sample generated file, a 16,001-sample file with signed extrema
and true zero, odd unknown chunks, data preceding fmt, 18-byte fmt, and empty
audio. PCM values and sample counts were preserved. Sixteen malformed or
unsupported variants were rejected before output creation, covering truncation,
container/RIFF size, unsupported formats, byte rate, odd PCM bytes, duplicate or
missing required chunks, oversized chunk sizes, missing padding and incomplete
chunk headers. An output path naming a directory correctly reported an I/O error.
These are manual checks, not a committed automated suite.
