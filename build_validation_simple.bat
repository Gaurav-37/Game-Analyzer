@echo off
echo ========================================
echo   Bloomberg Terminal Validation Suite
echo   Professional Quality Assurance
echo ========================================
echo.

echo [1/2] Building System Validator...
g++ -std=c++17 -O2 -march=native -static-libgcc -static-libstdc++ ^
    validate_system.cpp ^
    -o ValidationSuite.exe

if %ERRORLEVEL% neq 0 (
    echo ❌ Validation suite build failed!
    echo.
    echo Troubleshooting:
    echo 1. Ensure g++ compiler is available
    echo 2. Check that validate_system.cpp exists
    pause
    exit /b 1
)

echo ✅ System validator built successfully!
echo.

echo [2/2] Validation Suite Build Complete!
echo.
echo 📁 Generated Files:
echo   • ValidationSuite.exe - System validation runner
echo.
echo 🚀 Professional Quality Assurance Ready!
echo.
echo Usage:
echo   ValidationSuite.exe - Run all system validations
echo.
echo 📋 Validation Tests:
echo   ✅ Memory allocation and cleanup
echo   ✅ Thread execution and synchronization
echo   ✅ Performance timing benchmarks
echo   ✅ String processing capabilities
echo   ✅ File system I/O operations
echo.
echo 🎯 Production Ready: All validations must pass
echo.
pause
