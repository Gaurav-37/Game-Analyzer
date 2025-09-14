@echo off
echo ========================================
echo   Bloomberg Terminal Progressive Tests
echo   Safe Enterprise Testing System
echo ========================================
echo.

echo [1/3] Setting up Pure MinGW64 Environment...
set PATH=C:\msys64\mingw64\bin;%PATH%
set CPATH=C:\msys64\mingw64\include
set LIBRARY_PATH=C:\msys64\mingw64\lib

echo ✅ MinGW64 environment configured
echo.

echo [2/3] Building Progressive Test Framework...
C:\msys64\mingw64\bin\g++.exe -std=c++17 -O2 -march=native -fopenmp -flto -ffast-math ^
    -I"C:\msys64\mingw64\include" ^
    -I"C:\msys64\mingw64\include\opencv4" ^
    -I"C:\msys64\mingw64\include\tesseract" ^
    -DOPENCV_CUDA_AVAILABLE=0 ^
    src/test_framework.cpp src/progressive_test_runner.cpp ^
    src/performance_monitor.cpp src/cuda_support.cpp ^
    src/advanced_ocr.cpp src/game_analytics.cpp ^
    src/optimized_screen_capture.cpp src/thread_manager.cpp ^
    -o ProgressiveTestSuite.exe ^
    -L"C:\msys64\mingw64\lib" ^
    -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_dnn -lopencv_video ^
    -ltesseract -lleptonica -lboost_thread-mt -lboost_filesystem-mt ^
    -lgdi32 -luser32 -lkernel32 -lpsapi -lcomctl32 -ld3d11 -ldxgi -lole32 -ldwmapi -lmsimg32 ^
    -lws2_32 -lwinmm -loleaut32 -luuid -lcomdlg32 -ladvapi32 -static-libgcc -static-libstdc++

if %ERRORLEVEL% neq 0 (
    echo ❌ Progressive test suite build failed!
    echo.
    echo Troubleshooting:
    echo 1. Ensure MSYS2 is installed and configured
    echo 2. Install dependencies: pacman -S mingw-w64-x86_64-opencv mingw-w64-x86_64-tesseract-ocr mingw-w64-x86_64-boost
    echo 3. Check that all source files exist in src/ directory
    echo 4. Verify MinGW64 environment is clean (no Cygwin conflicts)
    pause
    exit /b 1
)

echo ✅ Progressive test framework built successfully!
echo.

echo [3/3] Testing the Progressive System...
echo Running Basic System Tests...
ProgressiveTestSuite.exe --basic

echo.
echo 🚀 Bloomberg Terminal Progressive Test Suite Ready!
echo.
echo 📋 Progressive Testing Strategy:
echo   🔴 --basic            - Basic system tests (always safe)
echo   🔴 --opencv           - OpenCV image processing tests
echo   🔴 --ocr              - OCR text recognition tests
echo   🔴 --screen-capture   - Screen capture system tests
echo   🔴 --game-analytics   - Game event detection tests
echo   🔴 --threading        - Multi-threading tests
echo   🔴 --performance      - Performance monitoring tests
echo   🔴 --integration      - Cross-component integration tests
echo   🔴 --all              - Complete test suite (may hang)
echo.
echo 🎯 Usage Examples:
echo   ProgressiveTestSuite.exe --basic            - Start with basic tests
echo   ProgressiveTestSuite.exe --opencv           - Test OpenCV functionality
echo   ProgressiveTestSuite.exe --ocr              - Test OCR system
echo   ProgressiveTestSuite.exe --help             - Show detailed help
echo.
echo 🏢 Enterprise Features:
echo   • Progressive component testing (avoid hanging)
echo   • Timeout protection and error handling
echo   • Individual component validation
echo   • Professional reporting with detailed metrics
echo   • Production readiness assessment
echo   • Detailed failure analysis and recommendations
echo.
echo 🎯 Production Ready: All progressive tests must pass
echo.
echo ✅ Pure MinGW64 Build - No Cygwin Conflicts!
echo.
pause
