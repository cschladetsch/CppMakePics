[CmdletBinding()]
param(
    [string]$BuildType = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$RootDirectory = $PSScriptRoot
$BuildDirectory = Join-Path $RootDirectory "build"

if ($Clean -and (Test-Path $BuildDirectory)) {
    Remove-Item -Recurse -Force $BuildDirectory
}

if (!(Test-Path $BuildDirectory)) {
    New-Item -ItemType Directory -Path $BuildDirectory | Out-Null
}

Set-Location $BuildDirectory

# Pass CMAKE_BUILD_TYPE as an initial cache definition before generator evaluation
cmake -G "Ninja" "-DCMAKE_BUILD_TYPE:STRING=$BuildType" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ $RootDirectory
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }

cmake --build .
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

Set-Location $RootDirectory