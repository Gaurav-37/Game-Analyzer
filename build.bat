@echo off
echo Building Game Analyzer...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build the project
cmake --build . --config Release

REM Copy executable to root directory
if exist bin\Release\GameAnalyzer.exe (
    copy bin\Release\GameAnalyzer.exe ..\GameAnalyzer.exe
    echo Build successful! GameAnalyzer.exe created.
) else (
    echo Build failed!
    pause
    exit /b 1
)

cd ..
echo.
echo Game Analyzer built successfully!
echo Run GameAnalyzer.exe to start the application.
pause
