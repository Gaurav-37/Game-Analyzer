@echo off
title Game Analyzer - Quick Download & Run
color 0E

echo.
echo ========================================
echo    Game Analyzer - Quick Start
echo ========================================
echo.
echo This will download and run Game Analyzer
echo without any installation required!
echo.

set /p choice="Download and run Game Analyzer? (Y/N): "
if /i "%choice%" neq "Y" (
    echo Download cancelled.
    pause
    exit /b 0
)

echo.
echo [1/3] Creating temporary directory...
if not exist "%TEMP%\GameAnalyzer" mkdir "%TEMP%\GameAnalyzer"
cd "%TEMP%\GameAnalyzer"

echo [2/3] Downloading Game Analyzer...
echo Please wait, this may take a few minutes...

REM Download the portable version
powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/your-repo/game-analyzer/releases/latest/download/GameAnalyzer.exe' -OutFile 'GameAnalyzer.exe'}"

if not exist "GameAnalyzer.exe" (
    echo.
    echo ERROR: Failed to download Game Analyzer.
    echo Please check your internet connection and try again.
    echo.
    echo Alternative download options:
    echo 1. Visit: https://github.com/your-repo/game-analyzer/releases
    echo 2. Download the latest release manually
    echo 3. Use the installer version instead
    pause
    exit /b 1
)

echo [3/3] Launching Game Analyzer...
echo.
echo Game Analyzer is ready to use!
echo.
echo Note: This is a temporary download.
echo For permanent installation, use the installer version.
echo.

start "" "GameAnalyzer.exe"

echo.
echo Game Analyzer launched successfully!
echo.
echo The application is now running.
echo You can close this window.
echo.
pause
