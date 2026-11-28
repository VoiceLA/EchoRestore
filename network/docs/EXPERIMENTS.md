# English

[Русская версия](#russian)

# Transmission scenarios and result checks

Use the [startup instructions](USAGE.md) to prepare the WAV and manifest,
then start the receiver, emulator, and sender in that order. The receive timeout
must cover startup time, the recording duration, and the maximum configured delay.
Stop the emulator after the receiver exits.

## No configured loss

Set `--loss 0 --delay-ms 0 --jitter-ms 0`. If all packets arrive before the cutoff,
the receiver reports `all_received`, the missing count is zero, and output PCM
matches source PCM exactly. WAV container bytes can differ because metadata
and chunk layout are not preserved.

A 16000-sample recording consists of 50 full frames. A 16001-sample recording
consists of 50 full frames and one final frame containing a single sample.
The output always preserves the sample count from the manifest.

## Complete configured loss

Set `--loss 1`. All datagrams received by the emulator are intentionally dropped.
For a nonempty session, the receiver stops at its timeout, writes a silent WAV
of the original length, and reports the whole signal as missing.

For a 16000-sample session, the missing range is
`{"start": 0, "length": 16000}` and all 50 expected packets are missing.
This length is known from the manifest, even when no audio packet arrives.

## Random loss, delay, and jitter

Use `--loss 0.05 --delay-ms 50 --jitter-ms 10 --seed 42` as an example.
Each incoming datagram has a 5% drop probability; surviving datagrams are
scheduled with a delay from 40 to 60 ms. Actual timing also depends on the OS.

The seed reproduces the emulator's drop decisions for the same incoming datagram
order. The observed missing fraction need not equal 5%: a finite sequence is
random, and OS drops or packets arriving after the cutoff can add missing data.
Configured dropping and missing-by-cutoff counts describe different events.

## Packet handling

- Reordered packets are stored at their original sample offsets. Reordering alone
  does not create missing audio if packets arrive before the cutoff.
- Duplicate packets do not increase the unique received count. The first copy
  wins; conflicting copies are counted separately.
- Truncated, oversized, unsupported, wrong-session, and out-of-bounds packets
  are rejected and counted as malformed.
- An absent final packet remains detectable from the manifest. For a 16000-sample
  session, a missing final frame covers `start=15680, length=320`.
- Packets waiting in the emulator queue are discarded on Ctrl+C or SIGTERM
  and counted as `discarded_on_shutdown`.

## Output consistency

Compare the JSON report and PCM samples using these rules:

- `received_packet_count + missing_packet_count = expected_packet_count`.
- `total_samples` equals the WAV sample count.
- Missing ranges have positive lengths, remain within `total_samples`, and do not
  overlap; adjacent gaps are merged.
- The sum of missing range lengths equals the number of samples in missing frames,
  accounting for a short final frame.
- Missing positions contain zeros in the output WAV. Known zero-valued samples
  are not classified as missing merely because they are silent.
- Every accepted frame retains its sample position and PCM values.
- `termination_reason` is `all_received` when all unique frames were accepted;
  otherwise it is `timeout`.
---

<a id="russian"></a>

# Русский

# Сценарии передачи и проверка результата

Подготовьте WAV и manifest по [инструкции запуска](USAGE.md#russian), затем
запустите приёмник, эмулятор и отправитель в таком порядке. Срок приёма должен
учитывать запуск процессов, длительность записи и максимальную настроенную
задержку. Останавливайте эмулятор после завершения приёмника.

## Без настроенных потерь

Задайте `--loss 0 --delay-ms 0 --jitter-ms 0`. Если все пакеты приходят до
завершения приёма, приёмник сообщает `all_received`, пропусков нет,
а выходной PCM точно совпадает с исходным. Байты WAV-контейнера могут отличаться,
поскольку метаданные и расположение чанков не сохраняются.

Запись на 16000 отсчётов состоит из 50 полных кадров. Запись на 16001 отсчёт —
из 50 полных кадров и последнего кадра с одним отсчётом.
Выходной файл всегда сохраняет число отсчётов из manifest.

## Полная настроенная потеря

Задайте `--loss 1`. Эмулятор намеренно отбрасывает все принятые датаграммы.
Для непустой сессии приёмник завершается по таймауту, записывает WAV исходной
длины с тишиной и отмечает весь сигнал как отсутствующий.

Для сессии на 16000 отсчётов диапазон пропуска равен
`{"start": 0, "length": 16000}`, отсутствуют все 50 ожидаемых пакетов.
Длина известна из manifest, даже если ни один аудиопакет не получен.

## Случайные потери, задержка и jitter

Пример параметров: `--loss 0.05 --delay-ms 50 --jitter-ms 10 --seed 42`.
Вероятность отбросить каждую входящую датаграмму составляет 5%; оставшиеся
датаграммы планируются к отправке с задержкой от 40 до 60 мс.
Фактическое время также зависит от ОС.

Seed воспроизводит решения эмулятора о потерях при одинаковом порядке входных
датаграмм. Наблюдаемая доля пропусков не обязана равняться 5%: конечная
последовательность случайна, а потери в ОС и доставка после срока приёма могут
добавлять пропуски. Настроенное отбрасывание и отсутствие к моменту завершения
приёма описывают разные события.

## Обработка пакетов

- Переставленные пакеты размещаются по исходным смещениям отсчётов. Изменение
  порядка само по себе не создаёт пропусков, если пакеты приходят до срока приёма.
- Дубликаты не увеличивают число уникальных полученных пакетов. Используется
  первая копия; конфликтующие копии учитываются отдельно.
- Усечённые, слишком большие, неподдерживаемые, чужие и выходящие за границы
  пакеты отклоняются и учитываются как malformed.
- Отсутствие последнего пакета обнаруживается по manifest. Для записи на 16000
  отсчётов пропуск последнего кадра имеет `start=15680, length=320`.
- Пакеты, ожидающие в очереди эмулятора, отбрасываются по Ctrl+C или SIGTERM
  и учитываются в `discarded_on_shutdown`.

## Согласованность выходных данных

Сопоставляйте JSON-отчёт и PCM по следующим правилам:

- `received_packet_count + missing_packet_count = expected_packet_count`.
- `total_samples` равно числу отсчётов WAV.
- Диапазоны пропусков имеют положительную длину, находятся внутри `total_samples`
  и не пересекаются; соседние пропуски объединяются.
- Суммарная длина диапазонов равна числу отсчётов в отсутствующих кадрах
  с учётом неполного последнего кадра.
- Пропущенные позиции заполнены нулями в выходном WAV. Полученные нулевые
  отсчёты не считаются пропусками только потому, что они означают тишину.
- Каждый принятый кадр сохраняет исходную позицию и значения PCM.
- `termination_reason` равно `all_received`, если приняты все уникальные кадры;
  в остальных случаях — `timeout`.
