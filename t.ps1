<#
.SYNOPSIS
    Run all unit tests.
.DESCRIPTION
    Builds the project (if needed) and executes DiffusionApp.exe --test.
    Exits with the test runner's failure count as the process exit code.
#>
[CmdletBinding()]
param(
    [string]$BuildType = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$Exe = Join-Path $BuildDir "DiffusionApp.exe"

# Optional clean
if ($Clean -and (Test-Path $BuildDir)) {
    Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
}

if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Build if needed
if (!(Test-Path $Exe) -or (Get-Item $Exe).LastWriteTime -lt (Get-Item "$Root/main.cpp").LastWriteTime) {
    Write-Host "[t.ps1] Building..."
    Push-Location $BuildDir
    try {
        cmake -G "Ninja" `
            "-DCMAKE_BUILD_TYPE:STRING=$BuildType" `
            -DCMAKE_C_COMPILER=clang `
            -DCMAKE_CXX_COMPILER=clang++ `
            $Root
        if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }

        cmake --build .
        if ($LASTEXITCODE -ne 0) { throw "Build failed." }
    }
    finally {
        Pop-Location
    }
} else {
    Write-Host "[t.ps1] Binary up to date."
}

# Run tests
Write-Host "[t.ps1] Running all tests..."
& $Exe --test
exit $LASTEXITCODE
