#include "ui_framework.h"
#include "main.h" // For RealGameAnalyzerGUI class

// ModernDialog Implementation
ModernDialog::ModernDialog(HWND parentWindow, RealGameAnalyzerGUI* parentApp, const std::string& title, const std::string& content, int width, int height) 
    : parent(parentApp), isDarkMode(parentApp->modernThemeEnabled) {
    
    // Create main dialog window
    dialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "STATIC", title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        parentWindow, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    if (!dialog) return;
    
    // Enable dark title bar
    if (isDarkMode) {
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    }
    
    // Create content area with proper padding
    contentArea = CreateWindow(
        "STATIC", content.c_str(),
        WS_VISIBLE | WS_CHILD | SS_LEFT | SS_NOPREFIX,
        30, 40, width - 60, height - 100,
        dialog, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    // Create modern OK button
    okButton = CreateWindow(
        "BUTTON", "OK",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
        (width - 100) / 2, height - 60, 100, 35,
        dialog, (HMENU)1, GetModuleHandle(nullptr), nullptr
    );
    
    // Apply modern fonts
    if (parentApp->hModernFont) {
        SendMessage(contentArea, WM_SETFONT, (WPARAM)parentApp->hModernFont, TRUE);
        SendMessage(okButton, WM_SETFONT, (WPARAM)parentApp->hModernFont, TRUE);
    }
    
    // Center dialog on screen
    centerDialog(width, height);
    
    // Set up custom window procedure
    SetWindowLongPtr(dialog, GWLP_USERDATA, (LONG_PTR)parentApp);
    SetWindowLongPtr(dialog, GWLP_WNDPROC, (LONG_PTR)modernDialogProc);
}

void ModernDialog::centerDialog(int width, int height) {
    RECT rect;
    GetWindowRect(dialog, &rect);
    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    SetWindowPos(dialog, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);
}

void ModernDialog::show() {
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.hwnd == dialog || IsChild(dialog, msg.hwnd)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (msg.message == WM_COMMAND && LOWORD(msg.wParam) == 1) break;
        if (msg.message == WM_CLOSE && msg.hwnd == dialog) break;
    }
    
    DestroyWindow(dialog);
}

LRESULT CALLBACK ModernDialog::modernDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RealGameAnalyzerGUI* pThis = (RealGameAnalyzerGUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // Fill with theme-appropriate background
            if (pThis && pThis->modernThemeEnabled) {
                FillRect(hdc, &rect, pThis->hBackgroundBrush);
            } else {
                FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            if (pThis && pThis->modernThemeEnabled) {
                SetBkColor(hdc, ModernTheme::DARK_BACKGROUND_PRIMARY);
                SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                return (LRESULT)pThis->hBackgroundBrush;
            } else {
                SetBkColor(hdc, ModernTheme::BACKGROUND_PRIMARY);
                SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                return (LRESULT)pThis->hBackgroundBrush;
            }
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            if (pThis && pThis->modernThemeEnabled) {
                SetBkColor(hdc, ModernTheme::DARK_BUTTON_NORMAL);
                SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                return (LRESULT)pThis->hCardBrush;
            } else {
                SetBkColor(hdc, ModernTheme::BUTTON_NORMAL);
                SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                return (LRESULT)pThis->hCardBrush;
            }
        }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            if (lpDrawItem->CtlType == ODT_BUTTON && pThis) {
                ModernUI::drawModernButton(pThis, lpDrawItem);
                return TRUE;
            }
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
            break;
        }
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ModernButton Implementation
ModernButton::ModernButton(HWND parentWindow, const std::string& buttonText, int posX, int posY, int w, int h, HMENU buttonId) 
    : parent(parentWindow), text(buttonText), x(posX), y(posY), width(w), height(h), id(buttonId) {
    
    button = CreateWindow(
        "BUTTON", text.c_str(),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
        x, y, width, height,
        parent, id, GetModuleHandle(nullptr), nullptr
    );
}

HWND ModernButton::getHandle() const { 
    return button; 
}

void ModernButton::setFont(HFONT font) { 
    SendMessage(button, WM_SETFONT, (WPARAM)font, TRUE); 
}

void ModernButton::setText(const std::string& newText) { 
    text = newText; 
    SetWindowText(button, text.c_str()); 
}

// ModernLabel Implementation
ModernLabel::ModernLabel(HWND parentWindow, const std::string& labelText, int posX, int posY, int w, int h, bool center) 
    : parent(parentWindow), text(labelText), x(posX), y(posY), width(w), height(h) {
    
    DWORD style = WS_VISIBLE | WS_CHILD | SS_LEFT;
    if (center) style |= SS_CENTER;
    
    label = CreateWindow(
        "STATIC", text.c_str(),
        style,
        x, y, width, height,
        parent, nullptr, GetModuleHandle(nullptr), nullptr
    );
}

HWND ModernLabel::getHandle() const { 
    return label; 
}

void ModernLabel::setFont(HFONT font) { 
    SendMessage(label, WM_SETFONT, (WPARAM)font, TRUE); 
}

void ModernLabel::setText(const std::string& newText) { 
    text = newText; 
    SetWindowText(label, text.c_str()); 
}

// ModernPanel Implementation
ModernPanel::ModernPanel(HWND parentWindow, int posX, int posY, int w, int h) 
    : parent(parentWindow), x(posX), y(posY), width(w), height(h) {
    
    panel = CreateWindow(
        "STATIC", "",
        WS_VISIBLE | WS_CHILD,
        x, y, width, height,
        parent, nullptr, GetModuleHandle(nullptr), nullptr
    );
}

HWND ModernPanel::getHandle() const { 
    return panel; 
}

void ModernPanel::setBackground(HBRUSH brush) {
    // Implementation for setting panel background
}

// ModernUI Utility Functions
namespace ModernUI {
    void drawModernButton(RealGameAnalyzerGUI* parent, LPDRAWITEMSTRUCT lpDrawItem) {
        HDC hdc = lpDrawItem->hDC;
        RECT rect = lpDrawItem->rcItem;
        
        // Determine button state
        bool isPressed = (lpDrawItem->itemState & ODS_SELECTED);
        bool isFocused = (lpDrawItem->itemState & ODS_FOCUS);
        bool isDisabled = (lpDrawItem->itemState & ODS_DISABLED);
        
        // Choose colors based on theme and state
        COLORREF bgColor, textColor, borderColor, shadowColor;
        
        if (parent->modernThemeEnabled) {
            if (isDisabled) {
                bgColor = RGB(35, 35, 35);
                textColor = RGB(100, 100, 100);
                borderColor = RGB(50, 50, 50);
                shadowColor = RGB(20, 20, 20);
            } else if (isPressed) {
                bgColor = ModernTheme::DARK_BUTTON_PRESSED;
                textColor = ModernTheme::DARK_TEXT_PRIMARY;
                borderColor = RGB(70, 70, 70);
                shadowColor = RGB(15, 15, 15);
            } else {
                bgColor = ModernTheme::DARK_BUTTON_NORMAL;
                textColor = ModernTheme::DARK_TEXT_PRIMARY;
                borderColor = RGB(70, 70, 70);
                shadowColor = RGB(20, 20, 20);
            }
        } else {
            if (isDisabled) {
                bgColor = RGB(245, 245, 245);
                textColor = RGB(150, 150, 150);
                borderColor = RGB(220, 220, 220);
                shadowColor = RGB(200, 200, 200);
            } else if (isPressed) {
                bgColor = ModernTheme::BUTTON_PRESSED;
                textColor = ModernTheme::TEXT_PRIMARY;
                borderColor = RGB(180, 180, 180);
                shadowColor = RGB(200, 200, 200);
            } else {
                bgColor = ModernTheme::BUTTON_NORMAL;
                textColor = ModernTheme::TEXT_PRIMARY;
                borderColor = RGB(200, 200, 200);
                shadowColor = RGB(220, 220, 220);
            }
        }
        
        // Create brushes
        HBRUSH bgBrush = CreateSolidBrush(bgColor);
        HBRUSH borderBrush = CreateSolidBrush(borderColor);
        HBRUSH shadowBrush = CreateSolidBrush(shadowColor);
        
        // Draw subtle shadow for depth
        RECT shadowRect = rect;
        shadowRect.left += 1;
        shadowRect.top += 1;
        shadowRect.right += 1;
        shadowRect.bottom += 1;
        FillRect(hdc, &shadowRect, shadowBrush);
        
        // Fill main background
        FillRect(hdc, &rect, bgBrush);
        
        // Draw clean border
        FrameRect(hdc, &rect, borderBrush);
        
        // Draw text with modern font
        char buttonText[256];
        GetWindowTextA(lpDrawItem->hwndItem, buttonText, sizeof(buttonText));
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textColor);
        
        // Apply modern font
        HFONT oldFont = (HFONT)SelectObject(hdc, parent->hModernFont);
        
        // Center text with better padding
        RECT textRect = rect;
        textRect.left += 12;
        textRect.right -= 12;
        textRect.top += 4;
        textRect.bottom -= 4;
        
        DrawTextA(hdc, buttonText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        // Restore font
        SelectObject(hdc, oldFont);
        
        // Clean up
        DeleteObject(bgBrush);
        DeleteObject(borderBrush);
        DeleteObject(shadowBrush);
    }
    
    void applyModernTheme(HWND window, RealGameAnalyzerGUI* parent) {
        // Apply modern theme to window
        if (parent->modernThemeEnabled) {
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        }
    }
    
    void createModernFonts(HFONT& modernFont, HFONT& boldFont, HFONT& headerFont) {
        // Create modern fonts
        modernFont = CreateFontA(
            -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI Variable"
        );
        
        boldFont = CreateFontA(
            -14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI Variable"
        );
        
        headerFont = CreateFontA(
            -18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI Variable"
        );
    }
    
    void updateThemeBrushes(RealGameAnalyzerGUI* parent) {
        // Clean up old brushes
        if (parent->hBackgroundBrush) DeleteObject(parent->hBackgroundBrush);
        if (parent->hPanelBrush) DeleteObject(parent->hPanelBrush);
        if (parent->hCardBrush) DeleteObject(parent->hCardBrush);
        if (parent->hButtonHoverBrush) DeleteObject(parent->hButtonHoverBrush);
        if (parent->hButtonPressedBrush) DeleteObject(parent->hButtonPressedBrush);
        if (parent->hBorderBrush) DeleteObject(parent->hBorderBrush);
        
        // Create new brushes based on current theme
        if (parent->modernThemeEnabled) {
            parent->hBackgroundBrush = CreateSolidBrush(ModernTheme::DARK_BACKGROUND_PRIMARY);
            parent->hPanelBrush = CreateSolidBrush(ModernTheme::DARK_PANEL_BACKGROUND);
            parent->hCardBrush = CreateSolidBrush(ModernTheme::DARK_CARD_BACKGROUND);
            parent->hButtonHoverBrush = CreateSolidBrush(ModernTheme::DARK_BUTTON_HOVER);
            parent->hButtonPressedBrush = CreateSolidBrush(ModernTheme::DARK_BUTTON_PRESSED);
            parent->hBorderBrush = CreateSolidBrush(ModernTheme::DARK_BORDER_PRIMARY);
        } else {
            parent->hBackgroundBrush = CreateSolidBrush(ModernTheme::BACKGROUND_PRIMARY);
            parent->hPanelBrush = CreateSolidBrush(ModernTheme::PANEL_BACKGROUND);
            parent->hCardBrush = CreateSolidBrush(ModernTheme::CARD_BACKGROUND);
            parent->hButtonHoverBrush = CreateSolidBrush(ModernTheme::BUTTON_HOVER);
            parent->hButtonPressedBrush = CreateSolidBrush(ModernTheme::BUTTON_PRESSED);
            parent->hBorderBrush = CreateSolidBrush(ModernTheme::BORDER_PRIMARY);
        }
    }
}
