# English

[Русская версия](#russian)

# Running the network pipeline

Full build and startup commands: [README](../../README.md).


## Emulator parameters

| Parameter | Default | Meaning |
| --- | --- | --- |
| `--loss` | `0` | Independent probability of dropping a datagram, from 0 to 1; `0.05` = 5% |
| `--delay-ms` | `0` | Base delay for surviving datagrams, 0…60000 ms |
| `--jitter-ms` | `0` | Delay variation of ±jitter, 0…60000 ms |
| `--seed` | `42` | Generator seed for reproducible decisions with the same input datagram order |

The drop decision is made first. For surviving packets the delay is
`max(0, delay + U[-jitter, +jitter))` ms. This delays every surviving packet;
it is not a probability of delaying a subset. With `delay=0, jitter=0`, forwarding
is immediate. `loss=1` drops everything. A 5% probability does not mean exactly
5% missing packets in a short file. Changing delay/jitter does not change the
drop decisions.

The emulator forwards complete UDP datagrams without modifying their contents;
it does not interpret WAV data or EchoRestore headers. A single queue scheduled
with a monotonic clock allows new packets to arrive while others await forwarding.
Jitter may change delivery order. The queue is bounded to 16 MiB of payload data
and 65536 datagrams; overflow is counted separately from configured dropping.
Actual delay accuracy depends on the OS scheduler.

Ctrl+C or SIGTERM stops the emulator and prints the counters:
`received`, `dropped`, `forwarded`, `failed_to_forward`, `queue_overflow`,
`discarded_on_shutdown`. Queued packets are discarded on shutdown.
The emulator does not detect the end of a WAV file; stop it after the receiver exits.

Addresses are numeric IPv4; ports range from 1 to 65535. For local runs, use
different emulator and receiver ports. IPv6 and DNS names are not currently supported.

## Sender and manifest

The canonical metadata preparation command, without transmitting audio:

```bash
./build/echorestore_sender --input examples/sample.wav --session-id 1 --write-manifest examples/sample.session
```

The `.session` format is versioned text with a fixed field order:

```text
ECHORESTORE_SESSION 1
session_id 1
sample_rate 16000
channels 1
format PCM_S16LE
total_samples 16000
frame_samples 320
expected_packet_count 50
```

The total sample count is derived from the WAV; no manual entry is needed.
Use the same WAV and session ID (default `1`) when sending.
Only the manifest is shared with the receiver. `--pace` sends frames at planned
20 ms intervals using a monotonic clock; without it, transmission has no deliberate
waits. For direct transmission without the emulator, use the receiver's port
as the sender's destination.

## Receiver and results

The receiver stops when all expected unique packets have arrived or the overall
`--timeout-ms` expires (default 10000 ms), including time spent waiting for the
sender. The manifest makes a missing final packet or a completely empty stream
detectable. Missing means not received before reception ended; packets arriving
after process exit cannot be counted. There is no separate late-packet metric
without a playout deadline.

Frames are stored by sample_offset. Availability is tracked separately from PCM,
where a real zero means silence. The first duplicate copy wins; conflicts are
counted separately. Invalid and wrong-session packets are counted as malformed.
`out_of_order_count` counts unique packets whose sequence is below the highest
previously received sequence.

JSON contains metadata, expected/received/missing_packet_count,
duplicate_count, conflicting_duplicate_count, out_of_order_count, malformed_count,
termination_reason (`all_received` / `timeout`), and missing_ranges.
Each range has `start` and `length` in samples; the end is exclusive.
Adjacent missing intervals are merged. This separate mask distinguishes missing
data from genuine silence in damaged.wav.

Directories are created automatically. WAV, manifest, and JSON outputs are replaced
using a temporary file named `<name>.tmp`; the WAV/JSON pair is not one transaction.
Use distinct paths, including temporary names, and one process per output file.
Receiving large files requires memory for the entire recording.
Sample counts are also limited by the 32-bit RIFF PCM16 size limit.

The pipeline transfers files offline. It does not provide streaming playback
or numerical recovery of missing samples.
---

<a id="russian"></a>

# Русский

# Запуск сетевой части

Полные команды сборки и запуска: [README](../../README.ru.md).


## Параметры эмулятора

| Параметр | Значение по умолчанию | Смысл |
| --- | --- | --- |
| `--loss` | `0` | Независимая вероятность отбросить датаграмму, от 0 до 1; `0.05` = 5% |
| `--delay-ms` | `0` | Базовая задержка оставшихся датаграмм, 0…60000 мс |
| `--jitter-ms` | `0` | Разброс задержки ±jitter, 0…60000 мс |
| `--seed` | `42` | Seed генератора для повторяемых решений при том же порядке входных датаграмм |

Сначала принимается решение о потере. Для оставшихся пакетов задержка равна
`max(0, delay + U[-jitter, +jitter))` мс. Это задержка всех неотброшенных пакетов,
а не вероятность задержать отдельную часть. При `delay=0, jitter=0` пересылка
немедленная. `loss=1` отбрасывает всё. Вероятность 5% не означает ровно 5%
потерь в коротком файле. Выбор потерь не меняется при изменении delay/jitter.

Эмулятор пересылает целые UDP-датаграммы без изменения содержимого; он не
интерпретирует WAV или заголовки EchoRestore. Одна очередь по монотонному
времени позволяет принимать новые пакеты, пока другие ждут отправки.
Jitter может изменить порядок доставки. Очередь ограничена 16 МиБ полезных
данных и 65536 датаграмм; переполнение учитывается отдельно от настроенных потерь.
Погрешность фактической задержки зависит от планировщика ОС.

Ctrl+C или SIGTERM завершает эмулятор и печатает счётчики:
`received`, `dropped`, `forwarded`, `failed_to_forward`, `queue_overflow`,
`discarded_on_shutdown`. Ожидающие в очереди пакеты при завершении отбрасываются.
Эмулятор не узнаёт о конце WAV; останавливайте его после завершения приёмника.

Адреса — числовые IPv4; порты 1…65535. Для локального запуска выбирайте разные
порты эмулятора и приёмника. IPv6 и имена DNS сейчас не поддерживаются.

## Отправитель и manifest

Каноническая подготовка метаданных без отправки:

```bash
./build/echorestore_sender --input examples/sample.wav --session-id 1 --write-manifest examples/sample.session
```

Формат `.session` — версионированный текст с фиксированным порядком полей:

```text
ECHORESTORE_SESSION 1
session_id 1
sample_rate 16000
channels 1
format PCM_S16LE
total_samples 16000
frame_samples 320
expected_packet_count 50
```

Общее число отсчётов определяется из WAV, вручную вводить его не нужно.
При отправке используйте тот же WAV и session ID (по умолчанию `1`).
Приёмнику передаётся только manifest. `--pace` отправляет кадры по запланированным
20-мс отметкам монотонных часов; без него отправка выполняется без ожиданий.
Для прямой передачи без эмулятора укажите отправителю порт приёмника.

## Приёмник и результаты

Приёмник заканчивает при получении всех ожидаемых уникальных пакетов либо
по общему `--timeout-ms` (по умолчанию 10000 мс), включая ожидание отправителя.
Пропущенный последний пакет и полностью пустой поток обнаруживаются по manifest.
Пропуск означает «не получен до завершения приёма»; пакеты после выхода процесса
не учитываются. Отдельной метрики поздних пакетов без срока воспроизведения нет.

Фреймы размещаются по sample_offset. Маска доступности хранится отдельно от PCM,
где настоящий ноль означает тишину. Из дубликатов используется первый; конфликты
считаются отдельно. Некорректные и чужие пакеты учитываются как malformed.
`out_of_order_count` считает уникальные пакеты с sequence меньше ранее максимального.

JSON содержит метаданные, expected/received/missing_packet_count,
duplicate_count, conflicting_duplicate_count, out_of_order_count, malformed_count,
termination_reason (`all_received` / `timeout`) и missing_ranges.
Каждый диапазон имеет `start` и `length` в отсчётах, конец не включён.
Соседние пропуски объединяются. Эта отдельная маска позволяет отличить
пропуски от настоящей тишины в damaged.wav.

Каталоги создаются автоматически. WAV, manifest и JSON заменяются через
временный файл `<имя>.tmp`; пара WAV/JSON не является общей транзакцией.
Используйте разные пути (включая временные имена) и один процесс на каждый
выходной файл. Приём больших файлов требует памяти под полную запись.
Число отсчётов ограничено также 32-битным размером RIFF PCM16.

Конвейер передаёт файлы в офлайн-режиме. Потоковое воспроизведение
и численное восстановление отсутствующих отсчётов не поддерживаются.
