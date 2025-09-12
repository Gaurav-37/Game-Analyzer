#include <windows.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "GameAnalyzer.h"
#include "external/imgui/backends/imgui_impl_win32.h"
#include "external/imgui/backends/imgui_impl_opengl3.h"

// Forward declare message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class GameAnalyzerApp {
private:
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
    std::unique_ptr<GameAnalyzer> analyzer;
    bool running;
    std::thread updateThread;

public:
    GameAnalyzerApp() : hwnd(nullptr), hdc(nullptr), hglrc(nullptr), running(false) {
        analyzer = std::make_unique<GameAnalyzer>();
    }

    ~GameAnalyzerApp() {
        cleanup();
    }

    bool initialize() {
        if (!createWindow()) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }

        if (!initializeOpenGL()) {
            std::cerr << "Failed to initialize OpenGL" << std::endl;
            return false;
        }

        if (!initializeImGui()) {
            std::cerr << "Failed to initialize ImGui" << std::endl;
            return false;
        }

        if (!analyzer->initialize()) {
            std::cerr << "Failed to initialize game analyzer" << std::endl;
            return false;
        }

        return true;
    }

    void run() {
        running = true;
        
        // Start update thread for non-blocking operations
        updateThread = std::thread([this]() {
            while (running) {
                analyzer->update();
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
            }
        });

        // Main message loop
        MSG msg = {};
        while (running && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

private:
    bool createWindow() {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = L"GameAnalyzer";
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassEx(&wc)) {
            return false;
        }

        hwnd = CreateWindowEx(
            0,
            L"GameAnalyzer",
            L"Game Analyzer - C++",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1200, 800,
            nullptr, nullptr,
            GetModuleHandle(nullptr),
            this
        );

        if (!hwnd) {
            return false;
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        return true;
    }

    bool initializeOpenGL() {
        hdc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;

        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        if (!pixelFormat) {
            return false;
        }

        if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
            return false;
        }

        hglrc = wglCreateContext(hdc);
        if (!hglrc) {
            return false;
        }

        if (!wglMakeCurrent(hdc, hglrc)) {
            return false;
        }

        // Initialize OpenGL extensions
        if (!gladLoadGL()) {
            return false;
        }

        return true;
    }

    bool initializeImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(hwnd)) {
            return false;
        }
        if (!ImGui_ImplOpenGL3_Init("#version 130")) {
            return false;
        }

        return true;
    }

    void cleanup() {
        running = false;
        
        if (updateThread.joinable()) {
            updateThread.join();
        }

        if (analyzer) {
            analyzer->shutdown();
        }

        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        // Cleanup OpenGL
        if (hglrc) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(hglrc);
        }
        if (hdc) {
            ReleaseDC(hwnd, hdc);
        }
        if (hwnd) {
            DestroyWindow(hwnd);
        }
    }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
            return true;
        }

        GameAnalyzerApp* app = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<GameAnalyzerApp*>(cs->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        } else {
            app = reinterpret_cast<GameAnalyzerApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        switch (msg) {
        case WM_SIZE:
            if (app && wParam != SIZE_MINIMIZED) {
                app->onResize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_DESTROY:
            if (app) {
                app->running = false;
            }
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void onResize(int width, int height) {
        glViewport(0, 0, width, height);
    }

    void render() {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render the game analyzer UI
        analyzer->render();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        SwapBuffers(hdc);
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Enable console for debugging in debug builds
    #ifdef _DEBUG
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    #endif

    try {
        GameAnalyzerApp app;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return -1;
        }

        app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
