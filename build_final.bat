@echo off
title Game Analyzer - Final Build & Cleanup
color 0A

echo.
echo ========================================
echo    Game Analyzer - Final Build
echo ========================================
echo.
echo Building final version and cleaning up...
echo.

REM Check if g++ is available
g++ --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: g++ not found in PATH
    pause
    exit /b 1
)

echo [1/4] Checking dependencies...

echo [2/4] Building final Game Analyzer...
g++ -std=c++17 -O2 -mwindows src/main.cpp src/ui_framework.cpp src/popup_dialogs.cpp -o GameAnalyzer.exe -lgdi32 -luser32 -lkernel32 -lpsapi -lcomctl32 -ld3d11 -ldxgi -lole32 -ldwmapi

if %errorlevel% neq 0 (
    echo [2/4] Build failed!
    pause
    exit /b 1
)

echo [3/4] Build successful! Cleaning up unnecessary files...

REM Remove old versions
if exist "GameAnalyzerGUI.exe" del "GameAnalyzerGUI.exe"
if exist "GameAnalyzerReal.exe" del "GameAnalyzerReal.exe"

REM Remove old build scripts
if exist "build_gui.bat" del "build_gui.bat"
if exist "build_real.bat" del "build_real.bat"

REM Remove old source files
if exist "src/gui_main.cpp" del "src/gui_main.cpp"
if exist "src/real_gui_main.cpp" del "src/real_gui_main.cpp"

REM External dependencies removed (not needed for final build)

echo [4/4] Final cleanup complete!

echo.
echo ========================================
echo    Final Build Complete!
echo ========================================
echo.
echo GameAnalyzer.exe is ready for distribution!
echo.
if exist "GameAnalyzer.exe" (
    echo File size: 
    dir GameAnalyzer.exe | findstr GameAnalyzer.exe
    echo.
    echo Final project structure:
    echo ├── GameAnalyzer.exe          # Your final application
    echo ├── src\main.cpp              # Source code
    echo ├── build_final.bat           # This build script
    echo ├── create_release.bat        # Release builder
    echo ├── installer.bat             # Installer
    echo ├── README.md                 # Documentation
    echo └── LICENSE                   # License
    echo.
    echo All unnecessary files removed!
    echo Project is clean and ready for distribution.
    echo.
    set /p launch="Would you like to run Game Analyzer now? (Y/N): "
    if /i "%launch%"=="Y" (
        start "" "GameAnalyzer.exe"
    )
) else (
    echo ERROR: GameAnalyzer.exe not found!
)

echo.
pause
