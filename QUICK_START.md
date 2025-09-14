# Game Analyzer - Build Instructions

## Bloomberg Terminal for Gaming Analytics

### Prerequisites

**System Requirements:**
- Windows 10/11 (x64)
- 8GB RAM minimum
- DirectX 11 compatible GPU
- MSYS2 development environment

### Build Process

1. **Install MSYS2:**
   ```bash
   # Download and install from: https://www.msys2.org/
   # Default installation path: C:\msys64
   ```

2. **Install Dependencies:**
   ```bash
   # Open MSYS2 MinGW64 terminal
   pacman -Sy
   pacman -S mingw-w64-x86_64-opencv
   pacman -S mingw-w64-x86_64-tesseract-ocr
   pacman -S mingw-w64-x86_64-boost
   pacman -S mingw-w64-x86_64-gcc
   ```

3. **Compile:**
   ```bash
   .\build_final.bat
   ```

## Architecture

### Core Components
- **Advanced OCR Engine**: Tesseract 5.5 with Leptonica preprocessing
- **Computer Vision**: OpenCV 4.12 with DNN module
- **Threading**: Boost.Thread with custom thread pool
- **Screen Capture**: DirectX 11 GPU-accelerated capture
- **Analytics**: Real-time game state analysis with financial metrics

### Performance Specifications
- **Processing Latency**: <50ms target
- **CPU Usage**: 6-10% with GPU acceleration
- **Memory**: Optimized frame caching
- **Threading**: Lock-free data structures where possible

## Runtime Requirements

**Required DLLs:**
- OpenCV modules (core, imgproc, dnn, video, highgui, imgcodecs)
- Tesseract OCR engine
- Leptonica image processing
- Boost threading and filesystem
- MinGW runtime (libgcc_s_seh-1.dll, libstdc++-6.dll)

## Launch

```bash
.\GameAnalyzer.bat
```

**Note:** See `PROBLEMS.md` for known issues and solutions.

