[CmdletBinding()]
param(
    [string]$Version = '6.8.3',
    [string]$OutputDirectory = 'C:\Qt',
    [string]$AqtVersion = '3.3.0'
)

$ErrorActionPreference = 'Stop'

$qtRoot = Join-Path $OutputDirectory "$Version\msvc2022_64"
$qmake = Join-Path $qtRoot 'bin\qmake.exe'

if (Test-Path -LiteralPath $qmake) {
    $installedVersion = & $qmake -query QT_VERSION
    if ($installedVersion -eq $Version) {
        Write-Host "Qt $Version is already installed at $qtRoot"
        exit 0
    }
}

if (-not (Get-Command python.exe -ErrorAction SilentlyContinue)) {
    throw 'Python 3 is required to install Qt with aqtinstall.'
}

python -m pip install --user "aqtinstall==$AqtVersion"
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to install aqtinstall.'
}

python -m aqt install-qt windows desktop $Version win64_msvc2022_64 --outputdir $OutputDirectory
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $qmake)) {
    throw "Qt installation failed: $qtRoot"
}

Write-Host "Qt $Version installed at $qtRoot"
Write-Host "Optional override for this shell: `$env:QT_ROOT='$qtRoot'"
