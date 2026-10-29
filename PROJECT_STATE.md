# NOXIS — состояние проекта

Фаза: Phase 1 реализована; рабочий standalone для Windows x64.
Стек: C++20, CMake, JUCE 8.0.16-security, commit ea77444750de45e036e0279bb0291144bf8e3688.

## Реализовано

- DSP без зависимости от JUCE: два осциллятора (sine, PolyBLEP saw/square,
  triangle с ограниченным набором гармоник), 8 голосов, ADSR, low-pass,
  сатурация, Output/Master, sustain CC64, voice stealing и panic.
- Сглаживание параметров, кроссфейд формы волны, атомарные целостные снимки
  пресетов, ограниченная MPSC-очередь нот, SPSC-буфер осциллографа.
- Нативный JUCE UI с собственным LookAndFeel, NOOB/PRO, реальными макросами
  Tone/Movement/Drive, числовым вводом, клавиатурой мыши/компьютера, MIDI-входом,
  выбором аудиоустройства, осциллографом и индикатором уровня, 30 FPS.
- Четыре заводских пресета в presets/, сохранение/загрузка .noxis.
  ROUTING показывает фиксированную цепь; FX управляет существующей сатурацией.
- Offline WAV renderer, тесты и скрипт scripts/build.ps1.

## Проверено

- MSVC 19.34 / Windows SDK 10.0.22000, Release x64: полная сборка и штатный скрипт.
- CTest: 2/2. Частоты четырёх форм при 44.1/48/96 kHz; ADSR, note on/off,
  полифония, stealing, sustain и MIDI-каналы; точный round-trip и идентичный
  звук пресетов, отклонение повреждённого файла; атомарность снимков,
  MPSC FIFO/переполнение/три источника, scope; стресс параметров при 22.05–96 kHz.
- renders/demo.wav: 12 s / 48 kHz / PCM16 mono, peak 0.0974286, RMS 0.0183811.
- Standalone smoke test: PASS, 343 callback, 165376 samples, peak 0.083806,
  0 голосов после release; Realtek, 48 kHz, 512 samples.
  Результаты: standalone-smoke.txt и standalone-smoke.wav (не отслеживаются Git).
- GUI запущен и осмотрен: PRO/NOOB, переключение Midnight Reese, изменение Release,
  виртуальная клавиша мышью и компьютерная A дают сигнал на scope/meter;
  завершение нот наблюдалось. Исправлены кириллица и формат значений.

## Ограничения

- Нативный диалог сохранения открылся, но Windows-автоматизация не смогла
  управлять им: failed to activate captured window. UI round-trip через диалог
  не подтверждён; файловый save/load и воспроизводимость звука проверены тестами.
- Аудиосигнал проверен в callback и WAV; акустический выход на слух не оценивался.
- Физический MIDI-контроллер, другие драйверы и переключение устройств не проверены.
- Выход dual mono. Нет LFO, Width/unison, pitch bend, delay/reverb/chorus,
  произвольного Routing, undo, автосохранения сессии/устройства, VST3/CLAP.
- Измерения алиасинга/CPU и длительные драйверные тесты относятся к следующему этапу.

## Следующие задачи

Ручная проверка файловых диалогов и внешнего MIDI на целевом оборудовании;
затем LFO и unison/стереопанорама. Подробности: ROADMAP.md.

## Сборка и запуск

```powershell
.\scripts\build.ps1
.\build\NOXIS_artefacts\Release\NOXIS.exe
.\build\Release\noxis_render.exe renders\demo.wav presets
```

Либо в Developer PowerShell for VS 2022:
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```
DSP без JUCE: ./scripts/build.ps1 -DspOnly.
При первой сборке нужен доступ к JUCE либо external/JUCE; готовое приложение офлайн.

## Git и публикация

Репозиторий: https://github.com/mifoxti/Project-NOXIS.
На GitHub опубликованы master, feature/dsp-engine и feature/standalone-ui.
Ветка по умолчанию — master; обе фича-ветки слиты в неё.
Автор новых коммитов для этого репозитория — mifoxti; сообщения — на русском.
Исходники, тесты, пресеты и документация отслеживаются Git; build/, external/
и сгенерированные WAV остаются локальными артефактами.
