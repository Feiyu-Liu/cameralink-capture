# Repository Guidelines

## Project Structure & Module Organization

This repository contains a Windows C++17 CameraLink capture application. Source and headers live at the repository root. `main.cpp` loads an INI and starts capture; `SaperaUse.*` owns the Sapera session; `FrameBuffer.*` maps Mono8 buffers into OpenCV; `CaptureWindow.*` tracks ring-buffer frame windows; `RealtimeView.*` handles preview and streaming; and `RecordFromBuffer.*` writes completed windows. Core tests are in `tests/CoreTests.cpp`. Hardware automation lives in `scripts/Invoke-HardwareAcceptance.ps1`. `opencv/`, `SaperaSDK/`, and `openh264-2.5.0/` are local dependencies excluded from Git; generated binaries belong under `x64/`.

## Build, Test, and Development Commands

Run from a Visual Studio 2022 Developer PowerShell:

```powershell
msbuild .\SaperaTest.sln /m /p:Configuration=Debug /p:Platform=x64
msbuild .\SaperaTest.sln /m /p:Configuration=Release1 /p:Platform=x64
.\x64\Debug\SaperaTest.CoreTests.exe
.\x64\Release1\SaperaTest.exe .\config.ini
```

The solution builds both the application and `SaperaTest.CoreTests`. Add the OpenCV and Sapera runtime `bin` directories to `PATH` before running binaries. Use `scripts/Invoke-HardwareAcceptance.ps1` only with a copied, machine-valid INI.

## Coding Style & Naming Conventions

Use four spaces, same-line braces, and C++17 standard-library facilities. Follow existing naming: PascalCase types (`FrameLayout`), camelCase functions and locals (`loadConfig`), `m_` for new private members, and uppercase compatibility macros such as `CONFIG`. Keep ownership explicit with RAII, references for required borrowed objects, and `noexcept` cleanup paths. Do not retain a `cv::Mat` after its mapped frame guard is destroyed.

## Testing Guidelines

Core tests use a small in-repository assertion harness; name cases `Test...` and keep them deterministic. Cover padded rows, invalid layouts and indexes, ring wraparound, tracker failures, and partial resource initialization. Run both Debug and Release1 tests. Capture changes also require a hardware run that records the selected board, CCF, layout, frame count, dropped/trash frames, and output metadata. Never edit the operator's original INI; create a temporary copy.

## Commit & Pull Request Guidelines

History favors concise release tags (`ava-8.1`) and short imperative subjects (`Update README.md`). Keep commits focused and subjects under 72 characters. PRs must describe behavior changes, tested configurations and hardware, affected INI keys, and linked issues. Attach logs or `ffprobe` metadata for recording changes and screenshots for preview changes. Never commit SDK binaries, recordings, build output, `.user` files, credentials, or machine-specific acceptance configs.
