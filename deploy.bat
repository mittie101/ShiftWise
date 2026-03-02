@echo off
setlocal EnableDelayedExpansion

:: ===========================================================================
::  ShiftWise — Full Deployment Pipeline
::  Walking Fish Software
::
::  Produces: installer\output\ShiftWise-1.0.0-Setup.exe
::
::  Prerequisites:
::    - Qt 6.10.2 MinGW installed at C:\Qt\
::    - Inno Setup 6 installed  https://jrsoftware.org/isinfo.php
:: ===========================================================================

set QT_DIR=C:\Qt\6.10.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set CMAKE=%USERPROFILE%\AppData\Local\Programs\Cmake\bin\cmake.exe
if not exist "%CMAKE%" set CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe
set NINJA=C:\Qt\Tools\Ninja\ninja.exe
set WINDEPLOYQT=%QT_DIR%\bin\windeployqt.exe
set INNO=C:\Program Files (x86)\Inno Setup 6\ISCC.exe

set BUILD_DIR=build\release
set DEPLOY_DIR=deploy

echo.
echo ============================================================
echo  ShiftWise Deployment Pipeline
echo ============================================================

:: MinGW must be on PATH before CMake tests the compiler
set PATH=%MINGW_DIR%\bin;%PATH%

:: ── Step 1: CMake Release configure ─────────────────────────────────────────
echo.
echo [1/4] Configuring Release build...
"%CMAKE%" -G "Ninja" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
    -DCMAKE_CXX_COMPILER="%MINGW_DIR%\bin\g++.exe" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -B %BUILD_DIR% -S .
if errorlevel 1 ( echo ERROR: CMake configure failed. & exit /b 1 )

:: ── Step 2: Build ───────────────────────────────────────────────────────────
echo.
echo [2/4] Building Release...
"%CMAKE%" --build %BUILD_DIR% --target ShiftWise
if errorlevel 1 ( echo ERROR: Build failed. & exit /b 1 )

:: ── Step 3: windeployqt ─────────────────────────────────────────────────────
echo.
echo [3/4] Collecting Qt dependencies...
if exist %DEPLOY_DIR% rmdir /s /q %DEPLOY_DIR%
mkdir %DEPLOY_DIR%
copy %BUILD_DIR%\ShiftWise.exe %DEPLOY_DIR%\ >nul

"%WINDEPLOYQT%" --release ^
    --no-translations ^
    --no-compiler-runtime ^
    --no-opengl-sw ^
    %DEPLOY_DIR%\ShiftWise.exe
if errorlevel 1 ( echo ERROR: windeployqt failed. & exit /b 1 )

:: MinGW runtime DLLs (not handled by windeployqt)
copy "%MINGW_DIR%\bin\libgcc_s_seh-1.dll"  %DEPLOY_DIR%\ >nul
copy "%MINGW_DIR%\bin\libstdc++-6.dll"     %DEPLOY_DIR%\ >nul
copy "%MINGW_DIR%\bin\libwinpthread-1.dll" %DEPLOY_DIR%\ >nul

:: ── Step 4: Inno Setup ──────────────────────────────────────────────────────
echo.
echo [4/4] Building installer...
if not exist "%INNO%" (
    echo WARNING: Inno Setup not found at "%INNO%"
    echo          Install from https://jrsoftware.org/isinfo.php
    echo          Then re-run this script or compile installer\shiftwise.iss manually.
    echo.
    echo Deploy folder is ready at: %DEPLOY_DIR%\
    exit /b 0
)
"%INNO%" installer\shiftwise.iss
if errorlevel 1 ( echo ERROR: Inno Setup compile failed. & exit /b 1 )

echo.
echo ============================================================
echo  Done!
echo  Installer: installer\output\ShiftWise-1.0.0-Setup.exe
echo ============================================================
echo.
