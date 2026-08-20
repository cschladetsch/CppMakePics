:: ===========================================================================
:: t.ps1 - Build and run all unit tests
:: ===========================================================================
@echo off
setlocal EnableDelayedExpansion

set "Root=C:\Users\chris\local\repos\CppMakePics"
set "BuildDir=%Root%\build"
set "Exe=%Root%\bin\tests\TestRunner.exe"
set "Sources=%Root%\src\main.cpp %Root%\src\image_gen.cpp %Root%\include\image_gen.hpp"

echo [t.ps1] Binary check...

if exist "%Exe%" (
    for %%F in ("%Exe%") do set "ExeTime=%%~tF"
    set "Rebuild=false"
    for %%S in (%Sources%) do (
        if exist "%%S" (
            for %%F in ("%%S") do (
                if "!ExeTime!" LSS "%%~tF" (
                    set "Rebuild=true"
                    goto :build
                )
            )
        )
    )
) else (
    set "Rebuild=true"
)

:build
if "%Rebuild%" == "true" (
    echo [t.ps1] Building...
    cd /d "%BuildDir%"
    cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "%Root%"
    if errorlevel 1 exit /b 1
    cmake --build . -- -j2
    if errorlevel 1 exit /b 1
    cd /d "%Root%"
) else (
    echo [t.ps1] Binary up to date.
)

echo [t.ps1] Running all tests...
"%Exe%"
exit /b %errorlevel%
