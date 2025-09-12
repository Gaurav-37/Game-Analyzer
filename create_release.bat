@echo off
title Game Analyzer - Release Builder
color 0B

echo.
echo ========================================
echo    Game Analyzer - Release Builder
echo ========================================
echo.
echo This script will create a distributable
echo version of Game Analyzer that users can
echo download and install like any other app.
echo.

REM Create release directory
if exist release rmdir /s /q release
mkdir release

echo [1/5] Building the application...
call build.bat
if not exist GameAnalyzer.exe (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo [2/5] Copying files to release directory...
copy GameAnalyzer.exe release\
copy README.md release\
copy LICENSE release\ 2>nul

REM Copy any required DLLs
if exist *.dll copy *.dll release\

echo [3/5] Creating portable version...
mkdir release\portable
copy GameAnalyzer.exe release\portable\
copy README.md release\portable\
if exist *.dll copy *.dll release\portable\

REM Create portable launcher
echo @echo off > release\portable\GameAnalyzer.bat
echo cd /d "%%~dp0" >> release\portable\GameAnalyzer.bat
echo GameAnalyzer.exe >> release\portable\GameAnalyzer.bat

echo [4/5] Creating installer...
if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
    "C:\Program Files (x86)\NSIS\makensis.exe" GameAnalyzer.nsi
    if exist GameAnalyzerInstaller.exe (
        move GameAnalyzerInstaller.exe release\
        echo Installer created successfully!
    )
) else (
    echo NSIS not found. Installer not created.
    echo Download NSIS from: https://nsis.sourceforge.io/
)

echo [5/5] Creating distribution package...
cd release
powershell -Command "& {Compress-Archive -Path * -DestinationPath GameAnalyzer-v1.0.zip -Force}"
cd ..

echo.
echo ========================================
echo    Release Created Successfully!
echo ========================================
echo.
echo Files created in 'release' directory:
echo - GameAnalyzer.exe (standalone executable)
echo - GameAnalyzerInstaller.exe (Windows installer)
echo - GameAnalyzer-v1.0.zip (complete package)
echo - portable\ (portable version)
echo.
echo You can now distribute these files to users!
echo.
echo Distribution options:
echo 1. Upload to GitHub Releases
echo 2. Share the ZIP file
echo 3. Use the installer for easy installation
echo 4. Portable version for no-install usage
echo.
pause
