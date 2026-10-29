# NOXIS — состояние проекта

Фаза: Phase 1, DSP проверен; standalone и интерфейс в работе.

Стек: C++20, CMake, JUCE 8.0.16-security, Windows x64.
DSP отделён от JUCE и собирается самостоятельно.

Реализовано: два осциллятора, PolyBLEP saw/square, ограниченная гармониками triangle,
8 голосов, ADSR, low-pass, сатурация, сглаживание, sustain и panic,
четыре пресета и строгая сериализация, WAV-рендерер, тесты.
Проверено: MSVC Release; CTest 2/2; WAV 12 s / 48 kHz PCM16, peak 0.0974286, RMS 0.0183811.
Известные ограничения: GUI и звуковое устройство ещё не проверены.
Следующие задачи: сборка standalone, визуальная проверка и проверка аудиовыхода.

Сборка в Developer PowerShell for VS 2022:
```
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\NOXIS_artefacts\Release\NOXIS.exe
```
Для DSP без JUCE: `cmake -S . -B build-dsp -A x64 -DNOXIS_BUILD_APP=OFF`.
