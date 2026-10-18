[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',
    [string]$BuildDirectory,
    [ValidateRange(1, 256)]
    [int]$Jobs = 2,
    [switch]$SkipSystemDeps,
    [string[]]$CMakeArgs = @()
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $root 'build/windows' }
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

function Install-Tool {
    param([string]$Id, [string[]]$ExtraArgs = @())
    if ($SkipSystemDeps) { throw "Missing $Id. Install it or run without -SkipSystemDeps." }
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        throw "Install Windows App Installer (winget), or install $Id manually."
    }
    Invoke-Checked winget (@('install', '--id', $Id, '--exact', '--silent',
        '--accept-package-agreements', '--accept-source-agreements') + $ExtraArgs)
    $env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' +
        [Environment]::GetEnvironmentVariable('Path', 'User')
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Install-Tool 'Git.Git' }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsPath = $null
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
if (-not $vsPath) {
    Install-Tool 'Microsoft.VisualStudio.2022.BuildTools' @('--override',
        '--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended')
    $vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'Visual C++ tools unavailable. Restart Windows if the installer requested it, then rerun.' }
}

function Get-CachedValue {
    param([string]$Name)
    $cache = Join-Path $BuildDirectory 'CMakeCache.txt'
    if (Test-Path -LiteralPath $cache) {
        $entry = Get-Content -LiteralPath $cache | Where-Object { $_ -match "^${Name}:[^=]+=" } | Select-Object -First 1
        if ($entry) { return ($entry -split '=', 2)[1] }
    }
}

# Reuse the build tree's generator explicitly, regardless of PATH or
# CMAKE_GENERATOR. A fresh tree uses the installed Visual Studio version.
$generator = Get-CachedValue 'CMAKE_GENERATOR'
if (-not $generator) {
    $vsVersion = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
    $vsMajor = ([version]$vsVersion).Major
}
elseif ($generator -notlike 'Visual Studio *') {
    throw "Build directory uses '$generator'. This script requires Visual Studio; select a separate directory with -BuildDirectory."
}

$pathCMake = Get-Command cmake -ErrorAction SilentlyContinue
$candidates = @(
    (Get-CachedValue 'CMAKE_COMMAND'),
    (Join-Path $vsPath 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')
)
if ($pathCMake) { $candidates += $pathCMake.Source }
if (-not ($candidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) })) {
    Install-Tool 'Kitware.CMake'
    $candidates += (Get-Command cmake -ErrorAction Stop).Source
}
$cmake = $null
foreach ($candidate in $candidates) {
    if (-not $candidate -or -not (Test-Path -LiteralPath $candidate)) { continue }
    $capabilities = & $candidate -E capabilities | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0 -or [version]$capabilities.version.string.Split('-')[0] -lt [version]'3.24') { continue }
    $supported = @($capabilities.generators | ForEach-Object { $_.name })
    if (-not $generator) {
        $generator = $supported | Where-Object { $_ -like "Visual Studio $vsMajor *" } | Select-Object -First 1
    }
    if ($generator -and $supported -contains $generator) {
        $cmake = $candidate
        break
    }
}
if (-not $cmake) {
    throw 'No CMake >= 3.24 supporting the selected Visual Studio version was found. Install or update the Visual Studio C++ CMake tools.'
}
Write-Host "Using CMake: $cmake"
Write-Host "Using generator: $generator"

# GLAD 1.x needs only Python's standard library. Keep a portable interpreter local
# to the build instead of depending on PATH or the Windows Store Python alias.
$env:PYTHONUTF8 = '1'
$python = Join-Path $BuildDirectory 'tools/python/python.exe'
if (-not (Test-Path $python)) {
    $pythonCommand = Get-Command python -ErrorAction SilentlyContinue
    if ($pythonCommand -and $pythonCommand.Source -notlike '*WindowsApps*') {
        $python = $pythonCommand.Source
    }
    elseif ($SkipSystemDeps) { throw 'Python 3 is missing. Install Python or run without -SkipSystemDeps.' }
    else {
        $pythonDirectory = Split-Path $python -Parent
        New-Item -ItemType Directory -Force -Path $pythonDirectory | Out-Null
        $archive = Join-Path $pythonDirectory 'python.zip'
        Invoke-WebRequest 'https://www.python.org/ftp/python/3.12.8/python-3.12.8-embed-amd64.zip' -OutFile $archive
        Expand-Archive -LiteralPath $archive -DestinationPath $pythonDirectory -Force
        Remove-Item -LiteralPath $archive
    }
}
# Embeddable Python uses isolated mode; GLAD must be on its module search path.
$pythonPathFile = Join-Path (Split-Path $python -Parent) 'python312._pth'
if ((Test-Path $pythonPathFile) -and $python -eq (Join-Path $BuildDirectory 'tools/python/python.exe')) {
    $gladSource = [IO.Path]::GetFullPath((Join-Path $BuildDirectory '_deps/glad-src'))
    Set-Content -LiteralPath $pythonPathFile -Value @('python312.zip', '.', $gladSource)
}

$configureArgs = @('-S', $root, '-B', $BuildDirectory, '-G', $generator, '-A', 'x64',
    "-DCMAKE_BUILD_TYPE=$Configuration", '-DOPENGL_FETCH_DEPENDENCIES=ON',
    '-DCMAKE_POLICY_VERSION_MINIMUM=3.5', "-DPython_EXECUTABLE=$python") + $CMakeArgs
Invoke-Checked $cmake $configureArgs
Invoke-Checked $cmake @('--build', $BuildDirectory, '--config', $Configuration, '--parallel', "$Jobs")
