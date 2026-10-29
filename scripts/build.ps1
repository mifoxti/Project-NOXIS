param([switch]$Run, [switch]$DspOnly)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$cmakeTool = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeTool) { $cmakePath = $cmakeTool.Source } else {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswherePath)) { throw 'Не найден CMake. Установите CMake и компоненты C++ Visual Studio 2022.' }
    $vsPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'Не найдены инструменты C++ Visual Studio.' }
    $cmakePath = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
$buildFolder = Join-Path $projectRoot $(if ($DspOnly) { 'build-dsp' } else { 'build' })
& $cmakePath -S $projectRoot -B $buildFolder -A x64 "-DNOXIS_BUILD_APP=$(if ($DspOnly) { 'OFF' } else { 'ON' })"
if ($LASTEXITCODE -ne 0) { throw 'Не удалось настроить проект.' }
& $cmakePath --build $buildFolder --config Release --parallel 4 -- /nodeReuse:false
if ($LASTEXITCODE -ne 0) { throw 'Ошибка сборки.' }
$ctestPath = Join-Path (Split-Path -Parent $cmakePath) 'ctest.exe'
& $ctestPath --test-dir $buildFolder -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Тесты не пройдены.' }
if ($Run -and -not $DspOnly) { Start-Process -FilePath (Join-Path $buildFolder 'NOXIS_artefacts\Release\NOXIS.exe') -WorkingDirectory $projectRoot -WindowStyle Normal }
