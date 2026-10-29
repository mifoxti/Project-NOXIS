# NOXIS — состояние проекта

Фаза: Phase 1, первоначальная реализация.

Стек: C++20, CMake, JUCE 8.0.16-security, Windows x64.
DSP отделён от JUCE и собирается самостоятельно.

Реализовано: каркас сборки; реализация находится в работе.
Известные ограничения: сборка и запуск ещё не проверены.
Следующие задачи: DSP, тесты и WAV; затем standalone, UI и пресеты.

Сборка в Developer PowerShell for VS 2022:
```
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\NOXIS_artefacts\Release\NOXIS.exe
```
Для DSP без JUCE: `cmake -S . -B build-dsp -A x64 -DNOXIS_BUILD_APP=OFF`.
