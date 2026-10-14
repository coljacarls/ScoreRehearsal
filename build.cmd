@echo off
rem SPDX-License-Identifier: GPL-3.0-only
rem ScoreRehearsal build script (Windows, MSVC + Ninja)
rem
rem usage: build.cmd [configure|build|install|run|clean] [Release|Debug|RelWithDebInfo] [score.mscz]
rem
rem   configure  run CMake only
rem   build      configure + compile
rem   install    configure + compile + install into build\<type>\install (default)
rem   run        install + start the app (optionally with a score file)
rem   clean      remove the build directory
rem
rem Environment: QT_DIR (default C:\Qt\6.11.2\msvc2022_64)

setlocal
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "ROOT_FWD=%ROOT:\=/%"

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=install"
set "BUILD_TYPE=%~2"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"
if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.11.2\msvc2022_64"
set "QT_DIR_FWD=%QT_DIR:\=/%"

rem "release" drops the "Development" suffix from the app name and data directory
set "APP_BUILD_MODE=dev"
if /I "%BUILD_TYPE%"=="Release" set "APP_BUILD_MODE=release"

set "BUILD_DIR=%ROOT%\build\%BUILD_TYPE%"
set "BUILD_DIR_FWD=%BUILD_DIR:\=/%"
set "INSTALL_DIR_FWD=%BUILD_DIR_FWD%/install"

if "%ACTION%"=="clean" (
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    goto :eof
)

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL_DIR=%%i"
if "%VS_INSTALL_DIR%"=="" (
    echo Visual Studio with the C++ toolchain was not found.
    exit /b 1
)
echo VS_INSTALL_DIR: %VS_INSTALL_DIR%
call "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%VS_INSTALL_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VS_INSTALL_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%QT_DIR%\bin;%PATH%"

cmake -S "%ROOT_FWD%/musescore" -B "%BUILD_DIR_FWD%" -G Ninja ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_PREFIX_PATH="%QT_DIR_FWD%" ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR_FWD%" ^
    -DMUSESCORE_BUILD_CONFIGURATION=app-external ^
    -DMUSE_APP_BUILD_MODE=%APP_BUILD_MODE% ^
    -DMUE_EXTERNAL_APP_DIR="%ROOT_FWD%/src" ^
    -DMUE_RUN_WINDEPLOYQT=ON ^
    -DMUSE_COMPILE_USE_UNITY=ON
if errorlevel 1 exit /b 1
if "%ACTION%"=="configure" goto :eof

ninja -C "%BUILD_DIR%"
if errorlevel 1 exit /b 1
if "%ACTION%"=="build" goto :eof

ninja -C "%BUILD_DIR%" install
if errorlevel 1 exit /b 1
if "%ACTION%"=="install" goto :eof

if "%ACTION%"=="run" start "" "%BUILD_DIR%\install\bin\ScoreRehearsal.exe" %3
