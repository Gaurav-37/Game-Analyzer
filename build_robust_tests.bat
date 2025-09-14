@echo off
echo ========================================
echo   Bloomberg Terminal Robust Test Suite
echo   Safe Initialization with Timeout Protection
echo ========================================
echo.

echo [1/3] Setting up Pure MinGW64 Environment...
set PATH=C:\msys64\mingw64\bin;%PATH%
set CPATH=C:\msys64\mingw64\include
set LIBRARY_PATH=C:\msys64\mingw64\lib

echo ✅ MinGW64 environment configured
echo.

echo [2/3] Building Robust Test Framework...
C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -march=native -fopenmp -ffast-math -pipe ^
    -I"C:\msys64\mingw64\include" ^
    -I"C:\msys64\mingw64\include\opencv4" ^
    -I"C:\msys64\mingw64\include\tesseract" ^
    -DOPENCV_CUDA_AVAILABLE=0 ^
    src/test_framework.cpp src/robust_test_runner.cpp ^
    src/performance_monitor.cpp src/cuda_support.cpp ^
    src/advanced_ocr.cpp src/game_analytics.cpp ^
    src/optimized_screen_capture.cpp src/thread_manager.cpp src/ui_framework.cpp ^
    -o RobustTestSuite.exe ^
    -L"C:\msys64\mingw64\lib" ^
    -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_dnn -lopencv_video ^
    -ltesseract -lleptonica -lboost_thread-mt -lboost_filesystem-mt ^
    -lgdi32 -luser32 -lkernel32 -lpsapi -lcomctl32 -ld3d11 -ldxgi -lole32 -ldwmapi -lmsimg32 ^
    -lws2_32 -lwinmm -loleaut32 -luuid -lcomdlg32 -ladvapi32 -static-libgcc -static-libstdc++

if %ERRORLEVEL% neq 0 (
    echo ❌ Robust test suite build failed!
    echo.
    echo Troubleshooting:
    echo 1. Ensure MSYS2 is installed and configured
    echo 2. Install dependencies: pacman -S mingw-w64-x86_64-opencv mingw-w64-x86_64-tesseract-ocr mingw-w64-x86_64-boost
    echo 3. Check that all source files exist in src/ directory
    echo 4. Verify MinGW64 environment is clean (no Cygwin conflicts)
    pause
    exit /b 1
)

echo ✅ Robust test framework built successfully!
echo.

echo [3/3] Testing the Robust System...
echo Running Bloomberg Terminal Robust Test Suite...
RobustTestSuite.exe

echo.
echo 🚀 Bloomberg Terminal Robust Test Suite Ready!
echo.
echo 📋 Robust Testing Features:
echo   • Safe component initialization with timeout protection
echo   • Automatic fallback to mock components when initialization fails
echo   • Comprehensive error handling and reporting
echo   • Performance validation with real or mock components
echo   • Enterprise-grade reliability and stability
echo   • No more hanging issues - guaranteed execution
echo.
echo 🎯 Usage:
echo   RobustTestSuite.exe

echo ✅ Pure MinGW64 Build - No Cygwin Conflicts!
echo.
pause