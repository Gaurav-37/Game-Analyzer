@echo off
title Game Analyzer - Setup Dependencies
color 0A

echo.
echo ========================================
echo    Game Analyzer - Setup Dependencies
echo ========================================
echo.
echo Setting up external dependencies...
echo.

REM Create external directory
if not exist "external" mkdir external
if not exist "external\imgui" mkdir external\imgui

echo [1/3] Downloading ImGui...
git clone https://github.com/ocornut/imgui.git external\imgui_temp
if %errorlevel% neq 0 (
    echo ERROR: Failed to download ImGui
    pause
    exit /b 1
)

echo [2/3] Copying required ImGui files...
copy "external\imgui_temp\*.cpp" "external\imgui\"
copy "external\imgui_temp\*.h" "external\imgui\"

echo [3/3] Cleaning up...
rmdir /s /q "external\imgui_temp"

echo.
echo ========================================
echo    Dependencies Setup Complete!
echo ========================================
echo.
echo You can now run build_final.bat to build the application.
echo.
pause
