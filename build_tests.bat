@echo off
echo ========================================
echo   Game Analyzer - Test Suite Builder
echo   Bloomberg Terminal Quality Assurance
echo ========================================
echo.

echo [1/3] Building Test Framework...
g++ -std=c++17 -O2 -march=native -fopenmp -flto -ffast-math ^
    -I"C:\msys64\mingw64\include" ^
    -I"C:\msys64\mingw64\include\opencv4" ^
    -I"C:\msys64\mingw64\include\tesseract" ^
    -DOPENCV_CUDA_AVAILABLE=0 ^
    src/test_framework.cpp src/test_runner.cpp ^
    src/performance_monitor.cpp src/cuda_support.cpp ^
    src/advanced_ocr.cpp src/game_analytics.cpp ^
    src/optimized_screen_capture.cpp src/thread_manager.cpp ^
    -o TestSuite.exe ^
    -L"C:\msys64\mingw64\lib" ^
    -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_dnn -lopencv_video ^
    -ltesseract -lleptonica -lboost_thread-mt -lboost_filesystem-mt ^
    -lgdi32 -luser32 -lkernel32 -lpsapi -lcomctl32 -ld3d11 -ldxgi -lole32 -ldwmapi -lmsimg32 ^
    -lws2_32 -lwinmm -loleaut32 -luuid -lcomdlg32 -ladvapi32 -static-libgcc -static-libstdc++

if %ERRORLEVEL% neq 0 (
    echo ❌ Test framework build failed!
    pause
    exit /b 1
)

echo ✅ Test framework built successfully!
echo.

echo [2/3] Building Benchmark Suite...
g++ -std=c++17 -O2 -march=native -fopenmp -flto -ffast-math ^
    -I"C:\msys64\mingw64\include" ^
    -I"C:\msys64\mingw64\include\opencv4" ^
    -I"C:\msys64\mingw64\include\tesseract" ^
    -DOPENCV_CUDA_AVAILABLE=0 ^
    src/benchmark_suite.cpp ^
    src/performance_monitor.cpp src/cuda_support.cpp ^
    src/advanced_ocr.cpp src/game_analytics.cpp ^
    src/optimized_screen_capture.cpp src/thread_manager.cpp ^
    -o BenchmarkSuite.exe ^
    -L"C:\msys64\mingw64\lib" ^
    -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_dnn -lopencv_video ^
    -ltesseract -lleptonica -lboost_thread-mt -lboost_filesystem-mt ^
    -lgdi32 -luser32 -lkernel32 -lpsapi -lcomctl32 -ld3d11 -ldxgi -lole32 -ldwmapi -lmsimg32 ^
    -lws2_32 -lwinmm -loleaut32 -luuid -lcomdlg32 -ladvapi32 -static-libgcc -static-libstdc++

if %ERRORLEVEL% neq 0 (
    echo ❌ Benchmark suite build failed!
    pause
    exit /b 1
)

echo ✅ Benchmark suite built successfully!
echo.

echo [3/3] Test Suite Build Complete!
echo.
echo 📁 Generated Files:
echo   • TestSuite.exe - Comprehensive test runner
echo   • BenchmarkSuite.exe - Performance benchmark suite
echo.
echo 🚀 Ready for Quality Assurance Testing!
echo.
echo Usage:
echo   TestSuite.exe [suite_name]  - Run specific test suite
echo   TestSuite.exe               - Run all tests
echo   BenchmarkSuite.exe          - Run performance benchmarks
echo.
pause
