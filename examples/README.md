# Sample / Образец

```bash
./build/echorestore_generate_sample examples/sample.wav
./build/echorestore_sender --input examples/sample.wav --write-manifest examples/sample.session
```

The generator writes a deterministic 1-second, 16 kHz mono PCM16 WAV:
16,000 samples of a 400 Hz square wave with amplitude ±8000. The sender derives
all metadata from it and writes a versioned text manifest; no audio is shared
through the manifest. Both commands create parent directories and overwrite outputs.

Генератор записывает детерминированный WAV: одна секунда, 16 кГц, моно PCM16,
прямоугольный сигнал 400 Гц амплитудой ±8000. Отправитель автоматически создаёт
manifest с метаданными и 50 ожидаемыми пакетами. Вручную переносить длину записи
не нужно. Это синтетический образец для проверки, не для оценки качества звука.

See [network usage](../network/docs/USAGE.md) and [запуск](../README.ru.md).
