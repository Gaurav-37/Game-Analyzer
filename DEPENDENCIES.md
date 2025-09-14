
# Game Analyzer - External Dependencies Installation Guide

## 🎯 Required Libraries for Full Bloomberg Terminal Features

### **Option 1: Automated Installation (Recommended)**
```bash
# Run the automated installer
.\install_dependencies.bat
```

### **Option 2: Manual Installation via MSYS2**

#### **Step 1: Install MSYS2**
1. Download MSYS2 from: https://www.msys2.org/
2. Install MSYS2 to default location
3. Open MSYS2 terminal

#### **Step 2: Update MSYS2**
```bash
pacman -Syu
```

#### **Step 3: Install OpenCV with CUDA Support**
```bash
# Core OpenCV
pacman -S mingw-w64-x86_64-opencv

# CUDA modules for GPU acceleration
pacman -S mingw-w64-x86_64-opencv-cuda
pacman -S mingw-w64-x86_64-opencv-cudaimgproc
pacman -S mingw-w64-x86_64-opencv-cudaobjdetect
pacman -S mingw-w64-x86_64-opencv-cudafeatures2d
```

#### **Step 4: Install Tesseract OCR**
```bash
# Tesseract OCR engine
pacman -S mingw-w64-x86_64-tesseract

# Language data files
pacman -S mingw-w64-x86_64-tesseract-data

# Leptonica (image processing library)
pacman -S mingw-w64-x86_64-leptonica
```

#### **Step 5: Install Additional Dependencies**
```bash
# Boost C++ libraries
pacman -S mingw-w64-x86_64-boost

# Development tools
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-gdb
pacman -S mingw-w64-x86_64-make

# Windows development libraries
pacman -S mingw-w64-x86_64-windows-default-manifest
```

### **Option 3: Alternative Installation Methods**

#### **Using vcpkg (Microsoft's C++ Package Manager)**
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install packages
.\vcpkg install opencv[contrib,cuda]:x64-windows
.\vcpkg install tesseract:x64-windows
.\vcpkg install boost:x64-windows
```

#### **Using Conan (C++ Package Manager)**
```bash
# Install Conan
pip install conan

# Create conanfile.txt
echo [requires] > conanfile.txt
echo opencv/4.8.0 >> conanfile.txt
echo tesseract/5.3.0 >> conanfile.txt
echo boost/1.82.0 >> conanfile.txt

# Install dependencies
conan install . --build=missing
```

## 🔧 **Library Details**

### **OpenCV (Open Source Computer Vision)**
- **Version**: 4.8.0+
- **Purpose**: 
  - GPU-accelerated image processing
  - Real-time screen capture optimization
  - Differential region capture
  - CUDA support for <50ms latency
- **Key Modules**:
  - `opencv_core`: Core functionality
  - `opencv_imgproc`: Image processing
  - `opencv_imgcodecs`: Image I/O
  - `opencv_highgui`: GUI components
  - `opencv_cuda`: GPU acceleration

### **Tesseract OCR**
- **Version**: 5.3.0+
- **Purpose**:
  - Extract text from game UI elements
  - Health, score, ammo detection
  - Game-specific text recognition
- **Features**:
  - Multiple language support
  - Custom training data
  - High accuracy for game text

### **Windows.Media.Ocr API**
- **Purpose**: GPU-accelerated OCR (preferred)
- **Advantages**:
  - Hardware acceleration
  - Better performance than Tesseract
  - Native Windows integration
- **Requirements**: Windows 10+ with Media Foundation

### **Boost C++ Libraries**
- **Version**: 1.82.0+
- **Purpose**:
  - Enhanced threading support
  - Smart pointers and memory management
  - Advanced algorithms
  - Cross-platform compatibility

## 🚀 **After Installation**

1. **Verify Installation**:
   ```bash
   # Check OpenCV
   pkg-config --modversion opencv4
   
   # Check Tesseract
   tesseract --version
   ```

2. **Build Full Version**:
   ```bash
   .\build_final.bat
   ```

3. **Test Features**:
   - Run GameAnalyzer.exe
   - Check if advanced OCR is working
   - Verify GPU acceleration is enabled
   - Test Bloomberg analytics dashboard

## 🎯 **Performance Targets**

With all dependencies installed, you should achieve:
- **<50ms total processing latency**
- **6-10% CPU usage** (with GPU acceleration)
- **Real-time game state detection**
- **Automatic game identification**
- **Bloomberg-style analytics**

## 🆘 **Troubleshooting**

### **Common Issues**:

1. **"opencv2/opencv.hpp: No such file or directory"**
   - Solution: Ensure OpenCV is properly installed and in PATH

2. **"tesseract.h: No such file or directory"**
   - Solution: Install Tesseract development headers

3. **CUDA not found**
   - Solution: Install CUDA toolkit or use CPU-only OpenCV

4. **Linker errors**
   - Solution: Ensure all libraries are in the correct architecture (x64)

### **Fallback Options**:
If you encounter issues, you can always use the simplified build:
```bash
.\build_simplified.bat
```

This will create a working version without the advanced features.
