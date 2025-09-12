cmake_minimum_required(VERSION 3.16)
project(GameAnalyzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release for better performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific optimizations
if(MSVC)
    add_compile_options(/O2 /Ob2 /Oi /Ot /Oy /GL /DNDEBUG)
    add_link_options(/LTCG)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-O3 -march=native -flto)
    add_link_options(-flto)
endif()

# Find required packages
find_package(OpenGL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(${CMAKE_SOURCE_DIR}/external/imgui/backends)
include_directories(${CMAKE_SOURCE_DIR}/external/implot)

# Source files
set(SOURCES
    src/main.cpp
    src/GameAnalyzer.cpp
    src/MemoryReader.cpp
    src/ProcessManager.cpp
    src/DataExporter.cpp
    src/ChartRenderer.cpp
    src/ConfigManager.cpp
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/implot/implot.cpp
    external/implot/implot_items.cpp
)

# Create executable
add_executable(GameAnalyzer ${SOURCES})

# Link libraries
target_link_libraries(GameAnalyzer
    ${OPENGL_LIBRARIES}
    psapi
    kernel32
    user32
    gdi32
    winspool
    shell32
    ole32
    oleaut32
    uuid
    comdlg32
    advapi32
    opengl32
)

# Compiler-specific options
if(MSVC)
    target_compile_options(GameAnalyzer PRIVATE /W4 /permissive-)
else()
    target_compile_options(GameAnalyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(GameAnalyzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    WIN32_EXECUTABLE TRUE
)

# Copy resources to output directory
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR}/bin)
