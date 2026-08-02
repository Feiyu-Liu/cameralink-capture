param(
    [Parameter(Mandatory = $true)]
    [string]$ConfigPath,

    [Parameter(Mandatory = $true)]
    [string]$LogPath,

    [ValidateSet('Quit', 'BufferedRecord', 'StreamingRecord', 'TtlCancel', 'TtlRecord')]
    [string]$Action = 'Quit',

    [ValidateRange(1, 100)]
    [int]$Cycles = 1,

    [ValidateRange(1, 300)]
    [int]$TimeoutSeconds = 90,

    [ValidateRange(100, 60000)]
    [int]$ActionDelayMilliseconds = 1000
)

$ErrorActionPreference = 'Stop'

if ($env:CAMERA_ACCEPTANCE_WORKER -ne '1') {
    $workerArguments = @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', $PSCommandPath,
        '-ConfigPath', $ConfigPath,
        '-LogPath', $LogPath,
        '-Action', $Action,
        '-Cycles', $Cycles,
        '-TimeoutSeconds', $TimeoutSeconds,
        '-ActionDelayMilliseconds', $ActionDelayMilliseconds
    )
    $env:CAMERA_ACCEPTANCE_WORKER = '1'
    try {
        & powershell.exe @workerArguments
        if ($LASTEXITCODE -ne 0) {
            throw "Hardware acceptance worker failed with exit code $LASTEXITCODE."
        }
    }
    finally {
        Remove-Item Env:\CAMERA_ACCEPTANCE_WORKER -ErrorAction SilentlyContinue
    }
    return
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$executable = Join-Path $repoRoot 'x64\Release1\SaperaTest.exe'
$opencvBin = Join-Path $repoRoot 'opencv\build\x64\vc16\bin'
$saperaBin = 'C:\Program Files\Teledyne DALSA\Sapera\Bin\Win64'

if (-not (Test-Path -LiteralPath $ConfigPath -PathType Leaf)) {
    throw "Configuration file does not exist: $ConfigPath"
}
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Release1 executable does not exist: $executable"
}

$logDirectory = Split-Path -Parent $LogPath
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null

if (-not ('HardwareAcceptanceConsoleInput' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

public static class HardwareAcceptanceConsoleInput
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct KeyEvent
    {
        [MarshalAs(UnmanagedType.Bool)] public bool KeyDown;
        public ushort RepeatCount;
        public ushort VirtualKeyCode;
        public ushort VirtualScanCode;
        public char UnicodeChar;
        public uint ControlKeyState;
    }

    [StructLayout(LayoutKind.Explicit)]
    private struct InputRecord
    {
        [FieldOffset(0)] public ushort EventType;
        [FieldOffset(4)] public KeyEvent Key;
    }

    [DllImport("kernel32.dll")] private static extern bool FreeConsole();
    [DllImport("kernel32.dll", SetLastError = true)] private static extern bool AttachConsole(uint processId);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr CreateFileW(string name, uint access, uint share, IntPtr security, uint creation, uint flags, IntPtr template);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool WriteConsoleInputW(IntPtr handle, InputRecord[] records, uint length, out uint written);

    private static IntPtr input = new IntPtr(-1);

    public static void Connect(uint processId)
    {
        FreeConsole();
        if (!AttachConsole(processId)) throw new Win32Exception();
        input = CreateFileW("CONIN$", 0xC0000000, 3, IntPtr.Zero, 3, 0, IntPtr.Zero);
        if (input == new IntPtr(-1)) throw new Win32Exception();
    }

    public static void Send(char value)
    {
        InputRecord down = new InputRecord();
        down.EventType = 1;
        down.Key.KeyDown = true;
        down.Key.RepeatCount = 1;
        down.Key.VirtualKeyCode = (ushort)Char.ToUpperInvariant(value);
        down.Key.UnicodeChar = value;
        InputRecord up = down;
        up.Key.KeyDown = false;
        uint written;
        if (!WriteConsoleInputW(input, new[] { down, up }, 2, out written) || written != 2)
            throw new Win32Exception();
    }
}
'@
}

function Wait-LogCount {
    param([string]$Pattern, [int]$Count, [datetime]$Deadline)
    do {
        if (Test-Path -LiteralPath $LogPath) {
            $content = Get-Content -LiteralPath $LogPath -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
            if (($content | Select-String -Pattern $Pattern -AllMatches).Matches.Count -ge $Count) {
                return
            }
        }
        if ($hostProcess.HasExited) {
            throw "Camera process exited before log pattern appeared: $Pattern"
        }
        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $Deadline)
    throw "Timed out waiting for log pattern: $Pattern"
}

function Test-LogCount {
    param([string]$Pattern, [int]$Count, [datetime]$Deadline)
    do {
        if (Test-Path -LiteralPath $LogPath) {
            $content = Get-Content -LiteralPath $LogPath -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
            if (($content | Select-String -Pattern $Pattern -AllMatches).Matches.Count -ge $Count) {
                return $true
            }
        }
        if ($hostProcess.HasExited) {
            throw "Camera process exited before log pattern appeared: $Pattern"
        }
        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $Deadline)
    return $false
}

$title = 'CameraAcceptance_' + [Guid]::NewGuid().ToString('N')
$arguments = if ($Cycles -gt 1) { ' "' + $ConfigPath + '" --cycles ' + $Cycles } else { ' "' + $ConfigPath + '"' }
$command = 'title ' + $title +
    ' && set "OPENCV_LOG_LEVEL=ERROR"' +
    ' && set "PATH=' + $opencvBin + ';' + $saperaBin + ';%PATH%"' +
    ' && "' + $executable + '"' + $arguments + ' > "' + $LogPath + '" 2>&1'

$hostProcess = Start-Process -FilePath 'cmd.exe' -ArgumentList @('/D', '/C', $command) `
    -WorkingDirectory $repoRoot -WindowStyle Minimized -PassThru
$cameraProcess = $null
$startDeadline = (Get-Date).AddSeconds(10)
do {
    $cameraProcess = Get-CimInstance Win32_Process |
        Where-Object { $_.ParentProcessId -eq $hostProcess.Id -and $_.Name -eq 'SaperaTest.exe' } |
        Select-Object -First 1
    if ($cameraProcess) { break }
    Start-Sleep -Milliseconds 100
} while ((Get-Date) -lt $startDeadline)

if (-not $cameraProcess) {
    throw 'Could not locate the camera process.'
}

try {
    [HardwareAcceptanceConsoleInput]::Connect([uint32]$cameraProcess.ProcessId)
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $workingSetBytes = @()
    $ttlTriggered = $null
    [HardwareAcceptanceConsoleInput]::Send('x')

    for ($cycle = 1; $cycle -le $Cycles; ++$cycle) {
        Wait-LogCount 'Buffer layout:' $cycle $deadline
        $workingSetBytes += (Get-Process -Id $cameraProcess.ProcessId).WorkingSet64
        Start-Sleep -Milliseconds $ActionDelayMilliseconds
        switch ($Action) {
            'BufferedRecord' {
                [HardwareAcceptanceConsoleInput]::Send('r')
                Start-Sleep -Milliseconds 100
                [HardwareAcceptanceConsoleInput]::Send('q')
            }
            'StreamingRecord' {
                [HardwareAcceptanceConsoleInput]::Send('r')
                Start-Sleep -Milliseconds $ActionDelayMilliseconds
                [HardwareAcceptanceConsoleInput]::Send('s')
                Start-Sleep -Milliseconds 500
                [HardwareAcceptanceConsoleInput]::Send('q')
            }
            'TtlCancel' {
                [HardwareAcceptanceConsoleInput]::Send('r')
                Wait-LogCount 'Waiting for TTL trigger' 1 $deadline
                Start-Sleep -Milliseconds $ActionDelayMilliseconds
                [HardwareAcceptanceConsoleInput]::Send('s')
                Wait-LogCount 'TTL recording stopped' 1 $deadline
                [HardwareAcceptanceConsoleInput]::Send('q')
            }
            'TtlRecord' {
                [HardwareAcceptanceConsoleInput]::Send('r')
                Wait-LogCount 'Waiting for TTL trigger' 1 $deadline
                $triggerDeadline = (Get-Date).AddMilliseconds($ActionDelayMilliseconds)
                $ttlTriggered = Test-LogCount 'TTL trigger received' 1 $triggerDeadline
                if ($ttlTriggered) {
                    Wait-LogCount 'Frames saved to:' 1 $deadline
                }
                [HardwareAcceptanceConsoleInput]::Send('s')
                Wait-LogCount 'TTL recording stopped' 1 $deadline
                [HardwareAcceptanceConsoleInput]::Send('q')
            }
            default {
                [HardwareAcceptanceConsoleInput]::Send('q')
            }
        }
    }

    $remaining = [Math]::Max(1, [int]($deadline - (Get-Date)).TotalMilliseconds)
    if (-not $hostProcess.WaitForExit($remaining)) {
        throw 'Camera process did not exit before the acceptance timeout.'
    }
    $result = [pscustomobject]@{
        ExitCode = $hostProcess.ExitCode
        CameraProcessId = $cameraProcess.ProcessId
        LogPath = $LogPath
        WorkingSetBytes = $workingSetBytes
        TtlTriggered = $ttlTriggered
    }
    $resultPath = $LogPath + '.result.json'
    $result | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $resultPath -Encoding UTF8
    $result
}
finally {
    $remainingCamera = Get-Process -Id $cameraProcess.ProcessId -ErrorAction SilentlyContinue
    if ($remainingCamera) {
        Stop-Process -Id $cameraProcess.ProcessId -Force
    }
    $remainingHost = Get-Process -Id $hostProcess.Id -ErrorAction SilentlyContinue
    if ($remainingHost) {
        Stop-Process -Id $hostProcess.Id -Force
    }
}
