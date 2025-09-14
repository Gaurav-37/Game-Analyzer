#include "ui_framework.h"
#include "main.h" // For RealGameAnalyzerGUI class
#include <algorithm>

// ModernDialog Implementation
ModernDialog::ModernDialog(HWND parentWindow, RealGameAnalyzerGUI* parentApp, const std::string& title, const std::string& content, int width, int height, bool showCancel) 
    : parent(parentApp), isDarkMode(parentApp ? parentApp->modernThemeEnabled : false), title(title), content(content), width(width), height(height), hasCancelButton(showCancel) {
    
    // Create main dialog window with enhanced styling (non-modal)
    dialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "STATIC", title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        parentWindow, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    if (!dialog) return;
    
    // Enable dark title bar immediately with aggressive forcing - always check current state
    bool currentDarkMode = (parent && parent->modernThemeEnabled);
    if (currentDarkMode) {
        BOOL darkMode = TRUE;
        
        // Apply dark mode multiple times to ensure it sticks
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        
        // Force multiple refreshes to ensure dark mode applies
        SetWindowPos(dialog, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        InvalidateRect(dialog, nullptr, TRUE);
        UpdateWindow(dialog);
        
        // Apply dark mode again after window is fully created
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        
        // Force title bar refresh with multiple timers
        SetTimer(dialog, 1, 50, nullptr);   // First refresh
        SetTimer(dialog, 2, 150, nullptr);  // Second refresh
        SetTimer(dialog, 3, 300, nullptr);  // Final refresh
    } else {
        BOOL darkMode = FALSE;
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        SetWindowPos(dialog, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
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
    SetWindowLongPtr(dialog, GWLP_USERDATA, (LONG_PTR)this);
    SetWindowLongPtr(dialog, GWLP_WNDPROC, (LONG_PTR)modernDialogProc);
}

// Destructor
ModernDialog::~ModernDialog() {
    if (dialog) {
        DestroyWindow(dialog);
        dialog = nullptr;
    }
}

// Enhanced dialog methods
void ModernDialog::show() {
    if (dialog) {
        // Refresh theme before showing
        refreshTheme();
        
        ShowWindow(dialog, SW_SHOW);
        UpdateWindow(dialog);
        SetForegroundWindow(dialog);
        
        // SIMPLE FIX: Always check parent's current theme state
        bool isParentDarkMode = false;
        if (parent) {
            isParentDarkMode = parent->modernThemeEnabled;
        }
        
        if (isParentDarkMode) {
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            SetWindowPos(dialog, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            InvalidateRect(dialog, nullptr, TRUE);
            UpdateWindow(dialog);
            
            // Additional timer-based refresh
            SetTimer(dialog, 4, 200, nullptr);
        } else {
            BOOL darkMode = FALSE;
            DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            SetWindowPos(dialog, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
}

void ModernDialog::hide() {
    if (dialog) {
        ShowWindow(dialog, SW_HIDE);
    }
}

void ModernDialog::setTitle(const std::string& newTitle) {
    title = newTitle;
    if (dialog) {
        SetWindowText(dialog, title.c_str());
    }
}

void ModernDialog::setContent(const std::string& newContent) {
    content = newContent;
    if (contentArea) {
        SetWindowText(contentArea, content.c_str());
    }
}

void ModernDialog::refreshTheme() {
    if (!dialog || !parent) return;
    
    // SIMPLE FIX: Always check parent's current theme state
    bool isParentDarkMode = parent->modernThemeEnabled;
    
    // Force refresh the entire dialog with current theme
    InvalidateRect(dialog, nullptr, TRUE);
    UpdateWindow(dialog);
    
    // Force title bar update
    if (isParentDarkMode) {
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    } else {
        BOOL darkMode = FALSE;
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    }
    SetWindowPos(dialog, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

// Theme updates handled during dialog creation

// Enhanced dialog drawing
void ModernDialog::drawDialogBackground(HDC hdc, RECT& rect, bool isDarkMode) {
    // Create solid background (simplified for compatibility)
    COLORREF bgColor = isDarkMode ? ModernTheme::DARK_BACKGROUND_PRIMARY : ModernTheme::BACKGROUND_PRIMARY;
    
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw subtle border
    HBRUSH borderBrush = CreateSolidBrush(isDarkMode ? ModernTheme::DARK_BORDER_PRIMARY : ModernTheme::BORDER_PRIMARY);
    FrameRect(hdc, &rect, borderBrush);
    DeleteObject(borderBrush);
}

void ModernDialog::drawModernButton(HDC hdc, RECT& rect, const std::string& text, bool isPressed, bool isHovered, bool isDarkMode) {
    // Determine button colors
    COLORREF bgColor, textColor, borderColor;
    
    if (isPressed) {
        bgColor = isDarkMode ? ModernTheme::DARK_BUTTON_PRESSED : ModernTheme::BUTTON_PRESSED;
    } else if (isHovered) {
        bgColor = isDarkMode ? ModernTheme::DARK_BUTTON_HOVER : ModernTheme::BUTTON_HOVER;
    } else {
        bgColor = isDarkMode ? ModernTheme::DARK_BUTTON_NORMAL : ModernTheme::BUTTON_NORMAL;
    }
    
    textColor = isDarkMode ? ModernTheme::DARK_TEXT_PRIMARY : ModernTheme::TEXT_PRIMARY;
    borderColor = isDarkMode ? ModernTheme::DARK_BORDER_PRIMARY : ModernTheme::BORDER_PRIMARY;
    
    // Fill button background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw border
    HBRUSH borderBrush = CreateSolidBrush(borderColor);
    FrameRect(hdc, &rect, borderBrush);
    DeleteObject(borderBrush);
    
    // Draw text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    
    RECT textRect = rect;
    textRect.left += 5;
    textRect.right -= 5;
    
    DrawTextA(hdc, text.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ModernPanel Implementation
ModernPanel::ModernPanel(HWND parentWindow, int posX, int posY, int w, int h, const std::string& panelTitle, bool border) 
    : x(posX), y(posY), width(w), height(h), parent(parentWindow), title(panelTitle), hasBorder(border), isDarkMode(false) {
    
    panel = CreateWindow(
        "STATIC", "",
        WS_VISIBLE | WS_CHILD | (hasBorder ? SS_SUNKEN : SS_NOTIFY),
        x, y, width, height,
        parent, nullptr, GetModuleHandle(nullptr), nullptr
    );
}

HWND ModernPanel::getHandle() const {
    return panel;
}

void ModernPanel::setBackground(HBRUSH brush) {
    if (panel) {
        SetClassLongPtr(panel, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
        InvalidateRect(panel, nullptr, TRUE);
    }
}

void ModernPanel::setTitle(const std::string& newTitle) {
    title = newTitle;
    if (panel) {
        SetWindowText(panel, title.c_str());
    }
}

void ModernPanel::setDarkMode(bool darkMode) {
    isDarkMode = darkMode;
    InvalidateRect(panel, nullptr, TRUE);
}

void ModernPanel::drawPanel(HDC hdc, RECT& rect) {
    COLORREF bgColor = isDarkMode ? ModernTheme::DARK_PANEL_BACKGROUND : ModernTheme::PANEL_BACKGROUND;
    COLORREF borderColor = isDarkMode ? ModernTheme::DARK_BORDER_PRIMARY : ModernTheme::BORDER_PRIMARY;
    
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw border if enabled
    if (hasBorder) {
        HBRUSH borderBrush = CreateSolidBrush(borderColor);
        FrameRect(hdc, &rect, borderBrush);
        DeleteObject(borderBrush);
    }
    
    // Draw title if present
    if (!title.empty()) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, isDarkMode ? ModernTheme::DARK_TEXT_PRIMARY : ModernTheme::TEXT_PRIMARY);
        
        RECT titleRect = rect;
        titleRect.top += 5;
        titleRect.bottom = titleRect.top + 20;
        
        DrawTextA(hdc, title.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

// ModernCard Implementation
ModernCard::ModernCard(HWND parentWindow, int posX, int posY, int w, int h, const std::string& cardTitle, const std::string& cardContent) 
    : x(posX), y(posY), width(w), height(h), parent(parentWindow), title(cardTitle), content(cardContent), isDarkMode(false), isHovered(false) {
    
    card = CreateWindow(
        "STATIC", "",
        WS_VISIBLE | WS_CHILD | SS_NOTIFY,
        x, y, width, height,
        parent, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    // Create title label
    titleLabel = CreateWindow(
        "STATIC", title.c_str(),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        x + 10, y + 10, width - 20, 25,
        parent, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    // Create content area
    contentArea = CreateWindow(
        "STATIC", content.c_str(),
        WS_VISIBLE | WS_CHILD | SS_LEFT | SS_EDITCONTROL,
        x + 10, y + 40, width - 20, height - 50,
        parent, nullptr, GetModuleHandle(nullptr), nullptr
    );
}

HWND ModernCard::getHandle() const {
    return card;
}

void ModernCard::setTitle(const std::string& newTitle) {
    title = newTitle;
    if (titleLabel) {
        SetWindowText(titleLabel, title.c_str());
    }
}

void ModernCard::setContent(const std::string& newContent) {
    content = newContent;
    if (contentArea) {
        SetWindowText(contentArea, content.c_str());
    }
}

void ModernCard::setDarkMode(bool darkMode) {
    isDarkMode = darkMode;
    InvalidateRect(card, nullptr, TRUE);
}

void ModernCard::setHovered(bool hovered) {
    isHovered = hovered;
    InvalidateRect(card, nullptr, TRUE);
}

void ModernCard::drawCard(HDC hdc, RECT& rect) {
    COLORREF bgColor = isDarkMode ? ModernTheme::DARK_CARD_BACKGROUND : ModernTheme::CARD_BACKGROUND;
    COLORREF borderColor = isDarkMode ? ModernTheme::DARK_BORDER_PRIMARY : ModernTheme::BORDER_PRIMARY;
    
    // Add hover effect
    if (isHovered) {
        bgColor = isDarkMode ? 
            RGB(GetRValue(bgColor) + 10, GetGValue(bgColor) + 10, GetBValue(bgColor) + 10) :
            RGB(std::max(0, (int)GetRValue(bgColor) - 10), std::max(0, (int)GetGValue(bgColor) - 10), std::max(0, (int)GetBValue(bgColor) - 10));
    }
    
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw border
    HBRUSH borderBrush = CreateSolidBrush(borderColor);
    FrameRect(hdc, &rect, borderBrush);
    DeleteObject(borderBrush);
    
    // Draw subtle shadow effect
    if (isHovered) {
        RECT shadowRect = rect;
        shadowRect.left += 2;
        shadowRect.top += 2;
        shadowRect.right += 2;
        shadowRect.bottom += 2;
        
        HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
        FrameRect(hdc, &shadowRect, shadowBrush);
        DeleteObject(shadowBrush);
    }
}

void ModernDialog::centerDialog(int width, int height) {
    RECT rect;
    GetWindowRect(dialog, &rect);
    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    SetWindowPos(dialog, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);
}


LRESULT CALLBACK ModernDialog::modernDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ModernDialog* dialog = (ModernDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // SIMPLE FIX: Always check parent's current theme state
            bool isParentDarkMode = false;
            if (dialog && dialog->parent) {
                isParentDarkMode = dialog->parent->modernThemeEnabled;
            }
            
            if (isParentDarkMode) {
                // Parent is in dark mode - use dark theme
                HBRUSH darkBrush = CreateSolidBrush(ModernTheme::DARK_BACKGROUND_PRIMARY);
                FillRect(hdc, &rect, darkBrush);
                DeleteObject(darkBrush);
                
                // Dark title bar
                BOOL darkMode = TRUE;
                DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            } else {
                // Parent is in light mode - use light theme
                FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
                
                // Light title bar
                BOOL darkMode = FALSE;
                DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER: {
            if (wParam == 1 || wParam == 2 || wParam == 3 || wParam == 4) {
                // SIMPLE FIX: Always check parent's current theme state
                bool isParentDarkMode = false;
                if (dialog && dialog->parent) {
                    isParentDarkMode = dialog->parent->modernThemeEnabled;
                }
                
                if (isParentDarkMode) {
                    BOOL darkMode = TRUE;
                    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
                    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
                } else {
                    BOOL darkMode = FALSE;
                    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
                    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
                }
                SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                InvalidateRect(hwnd, nullptr, TRUE);
                UpdateWindow(hwnd);
                
                // Kill the timer after use
                KillTimer(hwnd, wParam);
            }
            return 0;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            
            // SIMPLE FIX: Always check parent's current theme state
            bool isParentDarkMode = false;
            if (dialog && dialog->parent) {
                isParentDarkMode = dialog->parent->modernThemeEnabled;
            }
            
            if (isParentDarkMode) {
                // Parent is in dark mode - use dark theme
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                HBRUSH darkBrush = CreateSolidBrush(ModernTheme::DARK_BACKGROUND_PRIMARY);
                return (LRESULT)darkBrush;
            } else {
                // Parent is in light mode - use light theme
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                HBRUSH lightBrush = CreateSolidBrush(ModernTheme::BACKGROUND_PRIMARY);
                return (LRESULT)lightBrush;
            }
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            if (dialog && dialog->parent && dialog->parent->modernThemeEnabled) {
                // Dark theme
                SetBkColor(hdc, ModernTheme::DARK_BUTTON_NORMAL);
                SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                HBRUSH darkBrush = CreateSolidBrush(ModernTheme::DARK_BUTTON_NORMAL);
                return (LRESULT)darkBrush;
            } else {
                // Light theme
                SetBkColor(hdc, ModernTheme::BUTTON_NORMAL);
                SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                HBRUSH lightBrush = CreateSolidBrush(ModernTheme::BUTTON_NORMAL);
                return (LRESULT)lightBrush;
            }
        }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            if (lpDrawItem->CtlType == ODT_BUTTON && dialog && dialog->parent) {
                bool isDarkMode = dialog->parent->modernThemeEnabled;
                std::string buttonText = "OK";
                if (lpDrawItem->CtlID == IDCANCEL) buttonText = "Cancel";
                
                drawModernButton(lpDrawItem->hDC, lpDrawItem->rcItem, buttonText, 
                    lpDrawItem->itemState & ODS_SELECTED,
                    lpDrawItem->itemState & ODS_HOTLIGHT,
                    isDarkMode);
            }
            return TRUE;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                // Just hide the dialog, don't destroy it
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            break;
        }
        case WM_CLOSE: {
            // Just hide the dialog, don't destroy it
            ShowWindow(hwnd, SW_HIDE);
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
