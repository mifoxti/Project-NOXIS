# NOXIS — Phase 1

Standalone-синтезатор для Windows x64. C++20, JUCE 8.0.16-security
(`ea77444750de45e036e0279bb0291144bf8e3688`), CMake. DSP не зависит от JUCE.

## Сборка и запуск

Нужны Visual Studio 2022 с компонентами **Desktop development with C++**,
Windows SDK, CMake 3.22+ и Git. Откройте Developer PowerShell for VS 2022:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
.\build\NOXIS_artefacts\Release\NOXIS.exe
```

Либо запустите `./scripts/build.ps1` из PowerShell: скрипт найдёт CMake из Visual Studio,
соберёт приложение и выполнит тесты. `./scripts/build.ps1 -Run` также откроет NOXIS.

При первой настройке CMake скачивает закреплённый commit JUCE. Для сборки без доступа
к сети заранее поместите эту версию в `external/JUCE`. Готовое приложение работает офлайн.
Лицензия зависимости: `external/JUCE/LICENSE.md`.

Сборка только движка, без JUCE:

```powershell
cmake -S . -B build-dsp -A x64 -DNOXIS_BUILD_APP=OFF
cmake --build build-dsp --config Release
ctest --test-dir build-dsp -C Release --output-on-failure
```

## Управление

- Щёлкайте или перетаскивайте мышь по нижней клавиатуре.
- Ноты на компьютерной клавиатуре: **A W S E D F T G Y H U J K O L P ;**.
  Каждая следующая клавиша — следующий полутон. **Z/X** меняют октаву,
  **Escape / PANIC** отпускает все голоса. Используйте латинскую раскладку.
- В **Аудио** выбираются доступный Windows-драйвер, устройство, частота, буфер
  и вход MIDI. Задержка зависит от выбранного устройства и драйвера.
- **PRO** открывает все параметры. Перетаскивайте регулятор или вводите число
  в поле под ним. Двойной щелчок возвращает исходное значение.
- **NOOB** использует те же параметры: Tone → cutoff; Movement → противоположная
  расстройка OSC A/B; Drive → сатурация. Стереоширина пока отсутствует.
- Выбирайте Init, Midnight Reese, Afterglow Lead или Nightdrive Bass в верхнем
  списке либо на вкладке PRESETS. Звёздочка означает изменение параметров.
- **Сохранить / Загрузить** записывают и восстанавливают все 17 параметров
  в текстовом `.noxis`, включая мастер-громкость. Ошибочный файл не меняет звук.
- Осциллограф и индикатор показывают фактический выход движка, обновление 30 Гц.
- ROUTING показывает текущую фиксированную цепь. На FX доступны реальные Drive/Output.

## Offline WAV и тест устройства

```powershell
New-Item -ItemType Directory -Force renders
.\build\Release\noxis_render.exe renders\demo.wav presets
```

Рендерер использует тот же `Engine`, что standalone: 12 секунд, четыре пресета,
несколько одновременных нот, 48 kHz, PCM16 mono. До записи проверяются конечность
и уровни. Второй аргумент необязателен и экспортирует заводские `.noxis`.

```powershell
Start-Process -FilePath .\build\NOXIS_artefacts\Release\NOXIS.exe -ArgumentList '--smoke-test' -WorkingDirectory $PWD -Wait
```

Тест открывает GUI, играет короткий Reese-аккорд через выбранное по умолчанию
аудиоустройство, записывает `standalone-smoke.wav` и `standalone-smoke.txt`
в текущую папку и закрывает приложение. Проверяются реальные вызовы audio callback,
ненулевой сигнал и завершение release. Закройте обычный экземпляр перед тестом.
Проверка callback не доказывает, что акустическая система включена.

## Устройство проекта

`src/synth` — осцилляторы, огибающая, фильтр, голоса и сатурация;
`src/state` — параметры и пресеты; `src/audio` — JUCE I/O, очереди и WAV;
`src/ui` — нативные компоненты и LookAndFeel; `src/app` — жизненный цикл.

GUI публикует параметры атомарной транзакцией. Audio thread делает одну попытку
чтения и сохраняет предыдущий целостный снимок при конкурирующей записи.
Ноты передаются через ограниченную MPSC-очередь; переполнение включает panic.
Осциллограф использует заранее выделенный SPSC-буфер. Штатный callback не выполняет
файловых операций, выделений памяти, логирования, блокирующих mutex или вызовов GUI.

Поток: осцилляторы → фильтр/ADSR каждого голоса → сумма → сатурация → Output/Master.
Восемь голосов; освобождённые голоса крадутся первыми, затем самые старые.
Sustain CC64 поддерживается; pitch bend и MIDI CC automation пока отсутствуют.
Выход — dual mono; Windows-драйверы предоставляются JUCE, ASIO SDK не включён.

Следующие задачи перечислены в [ROADMAP.md](ROADMAP.md).
