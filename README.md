### Camera recorder for CameraLink camera based on Xtium-CL MX4

A simple video recorder that supports various trigger modes. During recording, video frames are first written into memory and then stored sequentially, enabling stable recording of high-speed videos on low-performance computers.

- Dependent Packages: https://pan.baidu.com/s/1BIn5T3Sz4CcS_lpkdoSP_A?pwd=xq6z

## Qt development environment

The GUI migration uses Qt 6.8.3 Widgets with the MSVC 2022 x64 toolchain. The existing
Visual Studio solution remains available for the console application.

```powershell
.\scripts\Install-Qt.ps1
.\scripts\Build-Qt.ps1 -Configuration Debug
.\scripts\Build-Qt.ps1 -Configuration Release
```

Qt is installed outside the repository at `C:\Qt\6.8.3\msvc2022_64`. Set `QT_ROOT`
or pass `-QtRoot` to `Build-Qt.ps1` to use another installation. CMake builds both
front ends: `SaperaTest.exe` remains the console application, while
`CameraLinkCapture.exe` is the Qt Widgets application with a camera preview and
command console. Runtime DLLs and the Qt platform plugin are copied beside each
Qt-linked executable by `windeployqt`.

Run the GUI with a valid hardware INI, or omit the argument to select one:

```powershell
.\build\msvc-x64\Debug\CameraLinkCapture.exe `
  C:\Users\BatLabWS\Desktop\MyBasler\config\config_cam1.ini
```

The GUI begins preview after initialization. Its terminal accepts `g`, `p`, `i`,
`r`, `s`, and `q`; `help` lists commands and `clear` clears only visible output.
The CMake test preset runs core capture tests, runtime tests, GUI tests, and the
Qt environment check. The Visual Studio solution remains the supported path for
the console application's Debug and Release1 hardware workflow.

Qt is dynamically linked. Before distributing the application, document the Qt
modules in use and comply with the selected LGPLv3 or commercial license terms.
