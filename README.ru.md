# EchoRestore

Простой конвейер на C++20: WAV → UDP-отправитель → эмулятор сети →
UDP-приёмник → повреждённый WAV и JSON с пропусками. [English](README.md).

Текущая цель — работающая сетевая часть с настройкой потерь, задержки и jitter.

## Сборка и запуск

Нужны Linux (в том числе WSL2), CMake 3.20+ и GCC/Clang с C++20.
Сторонних зависимостей нет.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/echorestore_generate_sample examples/sample.wav
./build/echorestore_sender --input examples/sample.wav --write-manifest examples/sample.session
```

В трёх терминалах запустите приёмник, эмулятор и отправитель в таком порядке:

```bash
./build/echorestore_receiver --bind 127.0.0.1 --port 9002 --manifest examples/sample.session --timeout-ms 10000 --output output/damaged.wav --report output/report.json
```

```bash
./build/echorestore_emulator --bind 127.0.0.1 --listen-port 9000 --forward-host 127.0.0.1 --forward-port 9002 --loss 0.05 --delay-ms 50 --jitter-ms 10 --seed 42
```

```bash
./build/echorestore_sender --input examples/sample.wav --host 127.0.0.1 --port 9000 --pace
```

`--loss 0.05` — вероятность отбросить каждый пакет 5%.
`--delay-ms 50 --jitter-ms 10` — задержка оставшихся пакетов от 40 до 60 мс.
По умолчанию все три значения равны нулю. Эмулятор останавливается по Ctrl+C.
Таймаут приёмника отсчитывается от привязки сокета: он должен учитывать время
запуска остальных процессов, длительность файла и максимальную задержку.
Файлы перезаписываются; используйте разные пути и один процесс на выходной файл.

Поддерживается WAV: 16 кГц, моно, signed PCM16. Пакет содержит 320 отсчётов
(20 мс), последний может быть короче. Приёмник сохраняет позиции отсчётов
и длину файла: пропуски записываются тишиной в WAV и отдельно диапазонами
`missing_ranges` в JSON. Manifest содержит только метаданные, без аудио.
Собственный протокол предназначен для доверенной локальной/тестовой сети, это не RTP.

## Структура

- `common/`: [WAV I/O](common/docs/WAV.md) и генератор, `echorestore::audio`.
- `network/`: кодек (`echorestore::protocol`), передача и эмулятор (`echorestore::network`), три небольших CLI-приложения.
- [Подробности запуска](network/docs/USAGE.md), [протокол](network/docs/PROTOCOL.md), [ручные проверки](network/docs/EXPERIMENTS.md).

У каждой утилиты есть `--help`. `-DECHORESTORE_BUILD_NETWORK=OFF` собирает только
общие WAV-утилиты. Тестовую инфраструктуру по прежней договорённости добавит команда.

## Форматирование

Для C/C++ используется стиль LLVM и **clang-format 18**. Форматтер нужен
только для разработки; запуск аудиоконвейера от него не зависит.

Обычный порядок работы: `git push` → дождаться автоформатирования и коммита
от GitHub Actions → `git pull`. Локальные Git hooks не используются.

При желании можно проверить и исправить формат отслеживаемых Git C/C++-файлов
вручную из корня репозитория:

```bash
sudo apt-get install clang-format-18

# Проверить форматирование
git ls-files -z -- '*.cpp' '*.h' '*.hpp' | xargs -0 -r clang-format-18 --dry-run --Werror

# Применить форматирование
git ls-files -z -- '*.cpp' '*.h' '*.hpp' | xargs -0 -r clang-format-18 -i
```

GitHub Actions проверяет каждый push в ветку и pull request. При push он
исправляет стиль, повторяет проверку и отправляет отдельный коммит форматирования;
после этого выполните `git pull`. Репозиторий должен разрешать Actions запись
содержимого и push в эту ветку. Если защита ветки запрещает push бота,
примените исправления локально. В pull request, включая fork, CI показывает diff
и завершается с ошибкой до исправления формата. Этот workflow не собирает
и не запускает аудиоконвейер.
