<#
.SYNOPSIS
    Clean Release build of HaikenAnime, packaged into a folder containing only the
    executable and its runtime dependencies.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts/build-release.ps1
#>
[CmdletBinding()]
param(
    [string]$QtDir = 'C:/Qt/6.8.3/msvc2022_64',
    [string]$Ninja = 'C:/Qt/Tools/Ninja/ninja.exe',
    [string]$BuildDir = 'cmake-build-release',
    [string]$OutputDir = 'dist'
)

$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)

function Invoke-Checked([string]$File, [string[]]$Arguments) {
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}

# Load the MSVC x64 toolchain unless the shell already has it (e.g. Developer PowerShell). A cl.exe on
# PATH is not enough: without INCLUDE/LIB it cannot find the standard headers.
if (-not ($env:VCToolsInstallDir -and $env:INCLUDE)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'Visual Studio with the C++ x64 tools was not found.' }
    Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}

# Clean build: start from an empty build directory.
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
Invoke-Checked cmake @('-S', '.', '-B', $BuildDir, '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_MAKE_PROGRAM=$Ninja",
    "-DCMAKE_PREFIX_PATH=$QtDir", '-DBUILD_TESTING=OFF')
Invoke-Checked cmake @('--build', $BuildDir)

# Package: executable + Qt/QML/MSVC runtime only.
if (Test-Path $OutputDir) { Remove-Item -Recurse -Force $OutputDir }
New-Item -ItemType Directory -Path $OutputDir | Out-Null
Copy-Item (Join-Path $BuildDir 'HaikenAnime.exe') $OutputDir
Invoke-Checked (Join-Path $QtDir 'bin/windeployqt.exe') @('--release', '--compiler-runtime', '--qmldir', 'resources/qml', (Join-Path $OutputDir 'HaikenAnime.exe'))

# windeployqt does not ship the DirectX Shader Compiler runtime.
$dxcDir = "${env:ProgramFiles(x86)}\Windows Kits\10\Redist\D3D\x64"
foreach ($dll in 'dxcompiler.dll', 'dxil.dll') {
    $source = Join-Path $dxcDir $dll
    if (Test-Path $source) { Copy-Item $source $OutputDir }
    else { Write-Warning "$dll was not found in $dxcDir" }
}

Write-Host "Release package ready: $(Resolve-Path $OutputDir)"
