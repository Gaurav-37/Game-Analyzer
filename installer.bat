@echo off
title Game Analyzer - Installer
color 0A

echo.
echo ========================================
echo    Game Analyzer - Easy Installer
echo ========================================
echo.
echo This installer will:
echo 1. Download the Game Analyzer executable
echo 2. Install it to your system
echo 3. Create desktop shortcut
echo 4. Add to Start Menu
echo.
echo No Visual Studio or development tools needed!
echo.

set /p choice="Do you want to install Game Analyzer? (Y/N): "
if /i "%choice%" neq "Y" (
    echo Installation cancelled.
    pause
    exit /b 0
)

echo.
echo [1/4] Creating installation directory...
if not exist "%PROGRAMFILES%\Game Analyzer" mkdir "%PROGRAMFILES%\Game Analyzer"
if not exist "%APPDATA%\Game Analyzer" mkdir "%APPDATA%\Game Analyzer"

echo [2/4] Downloading Game Analyzer...
echo This may take a few minutes depending on your internet speed...

REM Download the pre-built executable
powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/your-repo/game-analyzer/releases/latest/download/GameAnalyzer.exe' -OutFile '%PROGRAMFILES%\Game Analyzer\GameAnalyzer.exe'}"

if not exist "%PROGRAMFILES%\Game Analyzer\GameAnalyzer.exe" (
    echo.
    echo ERROR: Failed to download Game Analyzer.
    echo Please check your internet connection and try again.
    echo.
    echo Alternative: You can download manually from:
    echo https://github.com/your-repo/game-analyzer/releases
    pause
    exit /b 1
)

echo [3/4] Creating desktop shortcut...
powershell -Command "& {$WshShell = New-Object -comObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%USERPROFILE%\Desktop\Game Analyzer.lnk'); $Shortcut.TargetPath = '%PROGRAMFILES%\Game Analyzer\GameAnalyzer.exe'; $Shortcut.Save()}"

echo [4/4] Adding to Start Menu...
if not exist "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Game Analyzer" mkdir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Game Analyzer"
powershell -Command "& {$WshShell = New-Object -comObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%APPDATA%\Microsoft\Windows\Start Menu\Programs\Game Analyzer\Game Analyzer.lnk'); $Shortcut.TargetPath = '%PROGRAMFILES%\Game Analyzer\GameAnalyzer.exe'; $Shortcut.Save()}"

echo.
echo ========================================
echo    Installation Complete!
echo ========================================
echo.
echo Game Analyzer has been installed successfully!
echo.
echo You can now:
echo - Double-click the desktop shortcut
echo - Find it in the Start Menu
echo - Run it from: %PROGRAMFILES%\Game Analyzer\GameAnalyzer.exe
echo.
set /p launch="Would you like to launch Game Analyzer now? (Y/N): "
if /i "%launch%"=="Y" (
    start "" "%PROGRAMFILES%\Game Analyzer\GameAnalyzer.exe"
)

echo.
echo Thank you for installing Game Analyzer!
pause
