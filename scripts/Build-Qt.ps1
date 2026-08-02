[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [string]$QtRoot = 'C:\Qt\6.8.3\msvc2022_64'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

$cmake = (Get-Command cmake.exe -ErrorAction SilentlyContinue).Source
if (-not $cmake -and (Test-Path -LiteralPath $vswhere)) {
    $vsRoot = & $vswhere -latest -products * -property installationPath
    $cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
if (-not $cmake -or -not (Test-Path -LiteralPath $cmake)) {
    throw 'CMake was not found. Install the Visual Studio C++ CMake tools component.'
}
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest)) {
    throw "CTest was not found beside CMake: $ctest"
}
if (-not (Test-Path -LiteralPath (Join-Path $QtRoot 'lib\cmake\Qt6\Qt6Config.cmake'))) {
    throw "Qt 6 was not found at $QtRoot. Run scripts\Install-Qt.ps1 first."
}

$env:QT_ROOT = $QtRoot
$preset = 'msvc-' + $Configuration.ToLowerInvariant()

Push-Location $repoRoot
try {
    & $cmake --preset msvc-x64 -DCAMERALINK_QT_ROOT="$QtRoot"
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }

    & $cmake --build --preset $preset
    if ($LASTEXITCODE -ne 0) { throw 'CMake build failed.' }

    & $ctest --preset $preset
    if ($LASTEXITCODE -ne 0) { throw 'CTest failed.' }
}
finally {
    Pop-Location
}
