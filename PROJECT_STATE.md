# NOXIS — состояние проекта

Фаза: Phase 2, интеграция реализована; GUI QA прервана физической клавишей Escape.
Версия: 0.2.0. C++20 / JUCE 8.0.16-security / Windows x64.
Исходная точка: b700c3b (переписанный эквивалент исходного1043931); master не откатывался.

## Реализовано

- Сохранён Phase1 DSP; stereo-unison1..8 с нормализацией, spread/width/phase random.
- 2LFO (5форм), depth/phase/retrigger, BPM sync/cycles-per-beat, matrix pitch/cutoff/level/width,
  envelope->cutoff. Mono/legato/glide, поканальныйpitchbend, sub/noise иTuneB.
- Chorus, delay/ping-pong, FDN reverb, saturation: bypass/mix и параметры.
- Реальный DAG Routing: типизированные порты, топологический план внеcallback,
  безопасный блоковый снимок с параметрами, fade при смене, сохранение всех нодов/связей.
- RoutingEditor: drag/drop/reconnect/delete, выбор/инспектор, auto/reset/fit/zoom/pan,
  boundedUndoRedo, preview/hover/fade/activity. Невалидный draft явно обозначен, не применяется.
- 36пресетов:4старых+32новых; категории/описания/теги/характер/MIDI range,
  браузер/поиск/favorites/user/preview. Собственное происхождение: presets/SOURCES.md.
- .noxis v2 со строгой атомарной загрузкой и миграциейv1; старые4файлаv1 сохранены,
  все36v2 экспортированы в presets/Factory.
- Тёмное окноJUCE, переразметка SYNTH/FX/MODULATION/NOOB/PRO,
  stereo meter, реальныйscope/LFOactivity, ADSR и базоваяfilter response.
- ФизическаяFL-клавиатура: Q=C5=MIDI60, Z=C4=MIDI48, русскаяраскладка,
  root note/octave/KEYS, release приtext-focus/background, Panic.
- AudioHost: stereo callback, pitch bend, atomic epochs для исключения старых нот послеPanic.

## Проверено

- Исходная контрольная версия: 2/2 CTest до изменений.
- ИнтегрированныйDSP Release: 5/5 CTest (DSP_and_presets,Offline_WAV,routing,phase2,library).
- Полная GUI Release собрана; осмотрены SYNTH, тёмный title bar и визуализации.
- Реальное устройство Realtek, 48 kHz / 512 samples: smoke PASS, stereo difference
  energy 1.03759, peak 0.0323016, 0 голосов после release, xruns=0, CPU peak 7.09%.
- Stress: 8 нот × 8 unison, triangle и все FX: PASS, stereo difference energy 1.92266,
  peak 0.0417017, 0 голосов после release, xruns=0, CPU peak 48.69%.
  Это короткие проверки данного устройства, а не гарантия всех драйверов и буферов.
- Нулевые RT allocations, stereo/normalization/LFO/matrix/glide/bend/effects,
  отличиеfilter/saturation-order, atomicgraph snapshots, v1/v2 и библиотека36.
- renders/phase2-demo.wav:24s,48kHz,PCM16stereo,peak0.0687304,RMS0.00730139.
- Worst-case offline8нот×8unison/triangle/allFX: средняя нагрузка45–56%realtime,
  ноp99 блоков превышаетдедлайн обычного процесса. Данные: renders/benchmark/results.txt.
  Это неaudio-priority измерение; максимальная RT стабильность пока не подтверждена.

## Следующие действия

Продолжить GUI QA после разрешения пользователя на компьютерное управление:
resize/maximize/minimize, физическая Q в двух раскладках, drag/reconnect/undo Routing,
поиск/избранное/preview и файловый round-trip. Автотесты модели клавиатуры проходят,
но короткая синтетическая Q в GUI не подтвердила воспроизведение: проверить фокус
и события короче аудиоблока. Автоматизация была остановлена Escape до уточнения.
Закончить проверку интерфейса, затем слить feature/phase2-audio в master и опубликовать
обычным push. Историю master не переписывать.

## Команды

```powershell
.\scripts\build.ps1
.\build\NOXIS_artefacts\Release\NOXIS.exe
.\build\Release\noxis_render.exe renders\phase2-demo.wav presets\Factory
ctest --test-dir build -C Release --output-on-failure
```

Git: https://github.com/mifoxti/Project-NOXIS; новыеавторыmifoxti, сообщениярусские.
Текущаяветка feature/phase2-audio; этапA в feature/phase2-ui ужеслитвлокальныйmaster.
