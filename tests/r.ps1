<#
.SYNOPSIS
    Run the application in normal mode (image generation + SDL display).
.DESCRIPTION
    Builds the project (if needed) and executes DiffusionApp.exe without
    test flags. The app reads config.json, generates images from Pollinations,
    and opens an SDL3 window that refreshes images every 30 seconds.
#>
[CmdletBinding()]
param(
    [string]$BuildType = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$RepoRoot = Split-Path $Root -Parent
$BuildDir = Join-Path $RepoRoot "build"
$Exe = Join-Path $BuildDir "DiffusionApp.exe"

if ($Clean -and (Test-Path $BuildDir)) {
    Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
}

if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

if (!(Test-Path $Exe) -or (Get-Item $Exe).LastWriteTime -lt (Get-Item "$RepoRoot/main.cpp").LastWriteTime) {
    Write-Host "[r.ps1] Building..."
    Push-Location $BuildDir
    try {
        cmake -G "Ninja" `
            "-DCMAKE_BUILD_TYPE:STRING=$BuildType" `
            -DCMAKE_C_COMPILER=clang `
            -DCMAKE_CXX_COMPILER=clang++ `
            $RepoRoot
        if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }

        cmake --build .
        if ($LASTEXITCODE -ne 0) { throw "Build failed." }
    }
    finally {
        Pop-Location
    }
} else {
    Write-Host "[r.ps1] Binary up to date."
}

Write-Host "[r.ps1] Starting DiffusionApp (normal mode)..."
& $Exe
exit $LASTEXITCODE
