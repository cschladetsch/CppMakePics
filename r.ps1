<#
.SYNOPSIS
    Run the application in normal mode (image generation + SDL display).
.DESCRIPTION
    Builds the project (if needed) and executes DiffusionApp.exe without
    test flags. The app reads config.json, generates images locally,
    and opens an SDL3 window that refreshes images every 30 seconds.
#>
param([string]$BuildType = "Release", [switch]$Clean)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$Exe = [System.IO.Path]::Combine($Root, "bin", "DiffusionApp.exe")

if ($Clean -and (Test-Path $BuildDir)) {
    Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
}

if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

$Rebuild = $true
if (Test-Path $Exe) {
    $ExeTime = (Get-Item $Exe).LastWriteTime
    $Rebuild = $false
    $Sources = @(
        [System.IO.Path]::Combine($Root, "src", "main.cpp"),
        [System.IO.Path]::Combine($Root, "src", "image_gen.cpp"),
        [System.IO.Path]::Combine($Root, "include", "image_gen.hpp")
    )
    foreach ($s in $Sources) {
        if (Test-Path $s) {
            if ($ExeTime -lt (Get-Item $s).LastWriteTime) {
                $Rebuild = $true
                break
            }
        }
    }
}

if ($Rebuild) {
    Write-Host "[r.ps1] Building..."
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
    } finally {
        Pop-Location
    }
} else {
    Write-Host "[r.ps1] Binary up to date."
}

Write-Host "[r.ps1] Starting DiffusionApp (normal mode)..."
Start-Process -FilePath $Exe -PassThru -Wait -NoNewWindow | Out-Null
exit 0
