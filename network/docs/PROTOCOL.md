# English

[Русская версия](#russian)

# EchoRestore protocol v1

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

The network component uses a versioned out-of-band manifest containing session ID,
16,000 Hz, one channel, PCM_S16LE, total samples, 320 samples/frame and expected
packet count (ceiling division). Only metadata is shared with the receiver.
Stop when all unique packets arrive or a monotonic overall deadline expires;
missing means not received by cutoff, including an absent final packet.
The separate availability mask must distinguish missing audio from genuine zero
samples. The packet codec does not implement sockets, manifests, receive cutoffs
or assembly; those are implemented in the separate network component.
See [usage](USAGE.md) for the `.session` text format and receiver policy.

## Packet codec API

Include `echorestore/protocol/packet.hpp` and link `echorestore::protocol`.
This compiled library has no WAV, socket or numerical dependency.

```cpp
#include <echorestore/protocol/packet.hpp>

using namespace echorestore::protocol;
const SessionBounds session{42, 321};
const AudioPacket final_frame{42, 1, 320, {-1234}};
const auto bytes = encode_packet(final_frame, session);
const auto decoded = decode_packet(bytes, session);
```

`AudioPacket` is an owning in-memory object, not an on-wire struct. The codec
serializes every field explicitly. Sample count and payload length are derived
from the sample vector; magic, version, type and reserved values are fixed.
Native `int16_t` values are preserved exactly, including -32768 and 32767.

Both operations require `SessionBounds` with the expected session ID and total
samples, supplied from known metadata. This is only packet-validation context,
not a manifest format or manifest parser. Validate larger source lengths before
narrowing them to the 32-bit total. A zero-length session permits no packets.

Validation rejects wrong session IDs, zero or more than 320 samples, sequence/
offset disagreement, frames outside the session, short nonfinal frames, and
incorrect final lengths. Offset multiplication uses 64-bit arithmetic so an
overflowed 32-bit sequence product cannot pass validation.

Decoding additionally rejects bad magic, unsupported version/type, nonzero
reserved bits, truncated or oversized datagrams, inconsistent payload size, and
extra trailing bytes. Accepted datagrams are 26..664 bytes; a 24-byte header
without samples is invalid. All fields are validated before allocating sample
storage, which is bounded to 320 samples. Invalid inputs throw
`std::invalid_argument` with a diagnostic; allocation errors propagate normally.

Pass the full UDP datagram to the decoder. The receiver detects receive-buffer
truncation using `MSG_TRUNC` rather than passing an apparently valid prefix of
an oversized datagram.

The receiver assembler handles duplicates, ordering and availability.
The codec validates positions but does not maintain sequence or session state.
The final packet is identified from session length, not from an END message.

Build with `cmake -S . -B build` and `cmake --build build -j`. Networking is enabled
by default; `-DECHORESTORE_BUILD_NETWORK=OFF` builds only shared WAV tooling.
---

<a id="russian"></a>

# Русский

# Протокол EchoRestore v1

Это собственный учебный протокол для доверенных локальных и тестовых сетей,
не RTP. Проверка CRC на уровне приложения и криптографическая защита
целостности отсутствуют.

Заголовок занимает 24 байта и сериализуется явно; многобайтовые поля заголовка
имеют порядок байтов big-endian. Монофоническая полезная нагрузка PCM16 — little-endian.

| Смещение | Байты | Поле |
| --- | --- | --- |
| 0 | 4 | ASCII `ECHO` |
| 4 | 1 | version = 1 |
| 5 | 1 | type = 1 (аудио) |
| 6 | 2 | reserved = 0 |
| 8 | 4 | session_id |
| 12 | 4 | sequence, начиная с 0 |
| 16 | 4 | sample_offset |
| 20 | 2 | sample_count |
| 22 | 2 | payload_bytes = 2 × sample_count |

Кадры содержат 320 отсчётов, кроме последнего неполного кадра;
`sample_offset = sequence * 320`. Некорректные поля, длины и границы,
неподдерживаемые форматы и входы, превышающие предел 32-битных смещений,
должны отклоняться. Нельзя передавать структуры C++ как сырые байты.
Идентификатор сессии и диапазоны проверяются по заранее известному manifest.

Сетевая часть использует версионированный manifest вне UDP-аудиопотока:
идентификатор сессии, 16 000 Гц, один канал, PCM_S16LE, общее число отсчётов,
320 отсчётов на кадр и ожидаемое число пакетов (деление с округлением вверх).
Приёмнику передаются только метаданные. Приём заканчивается, когда получены
все уникальные пакеты либо истёк общий срок по монотонным часам.
Пропуск означает «не получен к моменту завершения приёма», в том числе для
последнего пакета. Отдельная маска доступности должна отличать пропущенное
аудио от настоящих нулевых отсчётов. Кодек пакетов не реализует сокеты, manifest,
ограничение времени приёма или сборку аудио; это реализовано в отдельной сетевой
библиотеке. Формат `.session` и политика приёмника описаны в [инструкции](USAGE.md).

## API кодека пакетов

Подключите `echorestore/protocol/packet.hpp` и CMake-цель `echorestore::protocol`.
Эта компилируемая библиотека не зависит от WAV, сокетов или численных методов.

```cpp
#include <echorestore/protocol/packet.hpp>

using namespace echorestore::protocol;
const SessionBounds session{42, 321};
const AudioPacket final_frame{42, 1, 320, {-1234}};
const auto bytes = encode_packet(final_frame, session);
const auto decoded = decode_packet(bytes, session);
```

`AudioPacket` — объект в памяти, владеющий своими данными, а не структура для
непосредственной передачи по сети. Кодек сериализует каждое поле явно. Число
отсчётов и длина полезной нагрузки вычисляются из вектора отсчётов; magic,
версия, тип и reserved фиксированы. Значения `int16_t` сохраняются точно,
включая -32768 и 32767.

Обе операции требуют `SessionBounds` с ожидаемым идентификатором сессии
и общим числом отсчётов из известных метаданных. Это только контекст проверки
пакета, а не формат или парсер manifest. Более длинные исходные записи нужно
проверять до преобразования длины к 32-битному типу. Сессия нулевой длины
не допускает ни одного пакета.

Проверка отклоняет неверный идентификатор сессии, нулевое число отсчётов
или число больше 320, несоответствие sequence и смещения, кадры вне сессии,
короткие непоследние кадры и неверную длину последнего кадра. Умножение для
расчёта смещения выполняется в 64-битном типе, поэтому переполнение
32-битного произведения sequence не позволит пройти проверку.

Декодирование также отклоняет неверный magic, неподдерживаемые версию или тип,
ненулевые reserved-биты, усечённые или слишком большие датаграммы,
несоответствие размера полезной нагрузки и лишние байты в конце.
Принимаются датаграммы размером 26..664 байта; 24-байтовый заголовок без
отсчётов недопустим. Все поля проверяются до выделения памяти под отсчёты;
буфер ограничен 320 отсчётами. Некорректный вход вызывает
`std::invalid_argument` с описанием ошибки; ошибки выделения памяти
передаются без изменений.

Передавайте декодеру целую UDP-датаграмму. Приёмник обнаруживает усечение
приёмного буфера через `MSG_TRUNC` и отклоняет слишком большую датаграмму,
не передавая декодеру её внешне корректный префикс.

Сборщик приёмника обрабатывает дубликаты, порядок доставки
и доступность отсчётов. Кодек проверяет позиции, но не хранит состояние
последовательности пакетов или сессии. Последний пакет определяется по длине
сессии, а не по сообщению END.

Сборка: `cmake -S . -B build` и `cmake --build build -j`. Сетевая часть включена
по умолчанию; `-DECHORESTORE_BUILD_NETWORK=OFF` оставляет только общие WAV-утилиты.
