# Protocol plan (not implemented in M0)

The authoritative v1 requirements are in [AGENTS.md](../../AGENTS.md).
This is a custom educational protocol, not RTP, for trusted local/test networks.
There is no application CRC or cryptographic integrity.

The header is 24 bytes, explicitly serialized; multibyte header fields are
big-endian. The PCM16 mono payload is little-endian.

| Offset | Bytes | Field |
| --- | --- | --- |
| 0 | 4 | ASCII `ECHO` |
| 4 | 1 | version = 1 |
| 5 | 1 | type = 1 (audio) |
| 6 | 2 | reserved = 0 |
| 8 | 4 | session_id |
| 12 | 4 | sequence, starting at 0 |
| 16 | 4 | sample_offset |
| 20 | 2 | sample_count |
| 22 | 2 | payload_bytes = 2 × sample_count |

Frames contain 320 samples except the final partial frame, with
`sample_offset = sequence * 320`. Reject invalid fields, lengths and boundaries,
unsupported formats, and inputs exceeding 32-bit offsets. Never transmit raw
C++ structs. Session ID and ranges must be checked against the known manifest.

M2 will introduce a versioned out-of-band manifest containing session ID,
16,000 Hz, one channel, PCM_S16LE, total samples, 320 samples/frame and expected
packet count (ceiling division). Only metadata is shared with the receiver.
Stop when all unique packets arrive or a monotonic overall deadline expires;
missing means not received by cutoff, including an absent final packet.
The separate availability mask must distinguish missing audio from genuine zero
samples. M0 changes none of these policies.
