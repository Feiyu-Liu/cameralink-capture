# Repository Guidelines

## Project Structure & Module Organization

This repository contains a Windows C++17 CameraLink capture application. Root sources implement the shared hardware core: `SaperaUse.*` owns the Sapera session; `FrameBuffer.*` maps Mono8 buffers into OpenCV; `CaptureWindow.*` tracks ring-buffer windows; `RealtimeView.*` handles preview/streaming; `CaptureRuntime.*` handles commands and UTF-8 logging; and `PreviewMailbox.*` safely transfers preview copies. `main.cpp` is the retained console entry point. `gui/` contains the Qt Widgets front end and its `main.cpp`. Tests live under `tests/`; hardware automation is `scripts/Invoke-HardwareAcceptance.ps1`. `opencv/`, `SaperaSDK/`, and `openh264-2.5.0/` are local dependencies excluded from Git; generated binaries belong under `x64/` or `build/`.

## Build, Test, and Development Commands

Run from a Visual Studio 2022 Developer PowerShell:

```powershell
msbuild .\SaperaTest.sln /m /p:Configuration=Debug /p:Platform=x64
msbuild .\SaperaTest.sln /m /p:Configuration=Release1 /p:Platform=x64
.\x64\Debug\SaperaTest.CoreTests.exe
.\x64\Release1\SaperaTest.exe .\config.ini
.\scripts\Build-Qt.ps1 -Configuration Debug
.\build\msvc-x64\Debug\CameraLinkCapture.exe .\config.ini
```

The solution builds the console application and `SaperaTest.CoreTests`. CMake additionally builds `CameraLinkCapture`, `CaptureRuntimeTests`, `CameraLinkCapture.GuiTests`, and `SaperaTest.QtEnvironmentTest`; run `scripts/Install-Qt.ps1` once if Qt 6.8.3 is missing. Add the OpenCV and Sapera runtime `bin` directories to `PATH` before running binaries. Use `scripts/Invoke-HardwareAcceptance.ps1` only with a copied, machine-valid INI.

## Coding Style & Naming Conventions

Use four spaces, same-line braces, and C++17 standard-library facilities. Follow existing naming: PascalCase types (`FrameLayout`), camelCase functions and locals (`loadConfig`), `m_` for new private members, and uppercase compatibility macros such as `CONFIG`. Keep ownership explicit with RAII, references for required borrowed objects, and `noexcept` cleanup paths. Do not retain a `cv::Mat` after its mapped frame guard is destroyed.

## Testing Guidelines

Core and runtime tests use a small in-repository assertion harness; name cases `Test...` and keep them deterministic. GUI tests use Qt Test and must not require hardware or display an additional OpenCV window. Cover padded rows, invalid layouts and indexes, ring wraparound, command cancellation, preview ownership, terminal history, and partial resource initialization. Run CMake Debug/Release tests plus Debug/Release1 console tests. Capture changes also require a hardware run that records the selected board, CCF, layout, frame count, dropped/trash frames, and output metadata. Never edit the operator's original INI; create a temporary copy.

## Commit & Pull Request Guidelines

History favors concise release tags (`ava-8.1`) and short imperative subjects (`Update README.md`). Keep commits focused and subjects under 72 characters. PRs must describe behavior changes, tested configurations and hardware, affected INI keys, and linked issues. Attach logs or `ffprobe` metadata for recording changes and screenshots for preview changes. Never commit SDK binaries, recordings, build output, `.user` files, credentials, or machine-specific acceptance configs.
