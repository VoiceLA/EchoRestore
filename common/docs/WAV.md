# English

[Русская версия](#russian)

# WAV I/O

`echorestore::audio` exposes `Pcm16Audio`, `read_wav(path)` and
`write_wav(path, audio)` through `echorestore/audio/wav.hpp`.
Link the CMake target `echorestore::audio`.

```cpp
#include <echorestore/audio/wav.hpp>

auto audio = echorestore::audio::read_wav("input.wav");
echorestore::audio::write_wav("output/copy.wav", audio);
```

The supported format is RIFF/WAVE format tag 1, 16,000 Hz, one channel,
signed 16-bit PCM. `Pcm16Audio::samples` holds native `int16_t` values in
[-32768, 32767], without normalization. Sample rate and channels are explicit
fields, validated on writing. On disk the samples and numeric fields are
little-endian, independently of host byte order. Zero represents actual silence;
sample availability is tracked separately by the receiver.

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
the maximum PCM16 sample count is `(UINT32_MAX - 36) / 2`. Reading a
very large valid file can still fail due to insufficient memory.

## Writer policies

Write a canonical 44-byte WAV header followed by the samples. Metadata and
unknown input chunks are not retained; PCM values are preserved exactly.
Validate format and the RIFF size limit before creating directories or opening
the destination. Create parent directories and overwrite an existing destination.
The write is not atomic: an I/O failure can leave a partial file. The receiver
writes to a temporary file and then renames it to publish the WAV output.

Invalid/unsupported WAV data and stream I/O failures raise `std::runtime_error`;
directory errors raise `std::filesystem::filesystem_error`. Allocation failures
propagate normally. Callers should include the input/output path in their error
context. No third-party dependency, UDP, resampling or interpolation is involved.

The sample generator uses this library's writer.
---

<a id="russian"></a>

# Русский

# Чтение и запись WAV

`echorestore::audio` предоставляет `Pcm16Audio`, `read_wav(path)` и
`write_wav(path, audio)` через заголовок `echorestore/audio/wav.hpp`.
Для подключения используйте CMake-цель `echorestore::audio`.

```cpp
#include <echorestore/audio/wav.hpp>

auto audio = echorestore::audio::read_wav("input.wav");
echorestore::audio::write_wav("output/copy.wav", audio);
```

Поддерживаемый формат: RIFF/WAVE, код формата 1, 16 000 Гц, один канал,
знаковый 16-битный PCM. `Pcm16Audio::samples` хранит обычные значения `int16_t`
в диапазоне [-32768, 32767] без нормализации. Частота дискретизации и число
каналов заданы отдельными полями и проверяются при записи. В файле отсчёты и
числовые поля имеют порядок байтов little-endian независимо от архитектуры
компьютера. Ноль означает настоящую тишину; приёмник хранит
доступность отсчётов отдельно.

## Правила чтения

- Чанки разбираются в пределах объявленной границы RIFF, без фиксированных
  смещений `fmt ` и `data`. Неизвестные чанки пропускаются вместе с байтом
  выравнивания при нечётной длине. Этот байт не обязан быть нулевым. Чанк `data`
  может предшествовать `fmt `: загрузка отсчётов откладывается до завершения разбора.
- Требуется ровно один чанк `fmt ` и один `data`. Дубликаты отклоняются как неоднозначные.
- Принимаются стандартный 16-байтовый PCM-чанк формата и 18-байтовый PCM
  WAVEFORMATEX с нулевым размером расширения. Другие расширения, включая
  WAVE_FORMAT_EXTENSIBLE, сжатое аудио, RF64 и RIFX, не поддерживаются.
- До загрузки аудио проверяются скорость потока в байтах, выравнивание блока,
  целостность PCM16-отсчётов, границы чанков, соответствие размера RIFF длине файла
  и допустимый размер буфера. Лишние байты за границей RIFF отклоняются,
  а не молча игнорируются.
- Пустой чанк `data` допустим и даёт пустой вектор отсчётов.

Файл загружается в память целиком, потоковой обработки нет. RIFF использует
32-битные размеры чанков; максимальное число отсчётов PCM16 равно
`(UINT32_MAX - 36) / 2`. Чтение очень большого корректного файла всё же
может завершиться ошибкой из-за нехватки памяти.

## Правила записи

Записывается стандартный 44-байтовый WAV-заголовок, за которым следуют отсчёты.
Метаданные и неизвестные входные чанки не сохраняются; значения PCM сохраняются
точно. Формат и ограничение размера RIFF проверяются до создания каталогов
и открытия файла. Родительские каталоги создаются, существующий файл
перезаписывается. Запись не атомарна: при ошибке ввода-вывода может остаться
неполный файл. Приёмник записывает WAV во временный файл, затем переименовывает
его для публикации результата.

Некорректные или неподдерживаемые WAV-данные и ошибки потокового ввода-вывода
вызывают `std::runtime_error`; ошибки каталогов —
`std::filesystem::filesystem_error`. Ошибки выделения памяти передаются вызывающему
коду без изменений. Вызывающий код должен добавлять путь входного или выходного
файла в контекст ошибки. Сторонних зависимостей, UDP, изменения частоты
дискретизации и интерполяции здесь нет.

Генератор образца использует функцию записи этой библиотеки.
