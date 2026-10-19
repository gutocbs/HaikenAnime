<#
.SYNOPSIS
    Validation build of HaikenAnime: MSVC Debug build with tests, followed by the full CTest run.
    Optionally also checks the MinGW configuration and produces the release package.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts/validate.ps1
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts/validate.ps1 -Clean -MinGW -Release
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts/validate.ps1 -Tests 'Sqlite'
#>
[CmdletBinding()]
param(
    [string]$QtDir = 'C:/Qt/6.8.3/msvc2022_64',
    [string]$Ninja = 'C:/Qt/Tools/Ninja/ninja.exe',
    [string]$BuildDir = 'cmake-build-validation',
    # Deletes the build directory first.
    [switch]$Clean,
    # Runs only the CTest suites whose names match this regular expression.
    [string]$Tests = '',
    # Configures a MinGW build without CMAKE_PREFIX_PATH to check that Qt is found through PATH.
    [switch]$MinGW,
    [string]$MinGWDir = 'C:/Qt/Tools/mingw1310_64',
    # Runs scripts/build-release.ps1 after the tests pass.
    [switch]$Release
)

$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)
$started = Get-Date
$summary = [System.Collections.Generic.List[string]]::new()

function Invoke-Checked([string]$File, [string[]]$Arguments) {
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}

# A cl.exe on PATH is not enough: without INCLUDE/LIB it cannot find the standard headers.
function Enter-MsvcShell {
    if ($env:VCToolsInstallDir -and $env:INCLUDE) { return }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'Visual Studio with the C++ x64 tools was not found.' }
    Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}

# MinGW first, before the MSVC environment is loaded into this shell.
if ($MinGW) {
    $mingwBuildDir = 'cmake-build-mingw-check'
    if (Test-Path $mingwBuildDir) { Remove-Item -Recurse -Force $mingwBuildDir }
    Invoke-Checked cmake @('-S', '.', '-B', $mingwBuildDir, '-G', 'Ninja', "-DCMAKE_MAKE_PROGRAM=$Ninja",
        "-DCMAKE_C_COMPILER=$MinGWDir/bin/gcc.exe", "-DCMAKE_CXX_COMPILER=$MinGWDir/bin/g++.exe",
        '-DCMAKE_BUILD_TYPE=Debug', '-DBUILD_TESTING=ON')
    $qtDirLine = Select-String -Path (Join-Path $mingwBuildDir 'CMakeCache.txt') -Pattern '^Qt6_DIR:' | Select-Object -First 1
    if (-not $qtDirLine -or $qtDirLine.Line -notmatch 'mingw') {
        throw "MinGW configure did not pick a MinGW Qt kit: $($qtDirLine.Line)"
    }
    $summary.Add("MinGW configure: OK ($($qtDirLine.Line))")
}

Enter-MsvcShell

if ($Clean -and (Test-Path $BuildDir)) { Remove-Item -Recurse -Force $BuildDir }
Invoke-Checked cmake @('-S', '.', '-B', $BuildDir, '-G', 'Ninja', "-DCMAKE_MAKE_PROGRAM=$Ninja",
    '-DCMAKE_BUILD_TYPE=Debug', "-DCMAKE_PREFIX_PATH=$QtDir", '-DBUILD_TESTING=ON')
Invoke-Checked cmake @('--build', $BuildDir, '--parallel')
$summary.Add('MSVC Debug build: OK')

$ctestArguments = @('--test-dir', $BuildDir, '--output-on-failure', '-j', '8')
if ($Tests) { $ctestArguments += @('-R', $Tests) }
& ctest @ctestArguments
$testsFailed = $LASTEXITCODE -ne 0
$summary.Add($(if ($testsFailed) { 'CTest: FAILED' } else { 'CTest: OK' }))

if ($Release -and -not $testsFailed) {
    Invoke-Checked powershell @('-ExecutionPolicy', 'Bypass', '-File', 'scripts/build-release.ps1', '-QtDir', $QtDir, '-Ninja', $Ninja)
    $summary.Add('Release package: OK (dist/)')
}

Write-Host "Executado em: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Write-Host ''
Write-Host '=== Validation summary ==='
$summary | ForEach-Object { Write-Host $_ }
Write-Host ("Elapsed: {0:mm\:ss}" -f ((Get-Date) - $started))
if ($testsFailed) { exit 1 }
