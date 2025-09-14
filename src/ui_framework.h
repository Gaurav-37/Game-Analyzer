#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <dwmapi.h>

// Forward declaration
class RealGameAnalyzerGUI;

// Modern UI Theme Colors - Professional Design System
namespace ModernTheme {
    // Light Theme - VS Code/Steam inspired
    const COLORREF BACKGROUND_PRIMARY = RGB(255, 255, 255);   // Pure white
    const COLORREF BACKGROUND_SECONDARY = RGB(248, 248, 248); // Light gray
    const COLORREF PANEL_BACKGROUND = RGB(243, 243, 243);     // Panel background
    const COLORREF CARD_BACKGROUND = RGB(255, 255, 255);      // Card background
    const COLORREF RIBBON_BACKGROUND = RGB(250, 250, 250);    // Ribbon background
    
    // Dark Theme - Ultra Modern Professional
    const COLORREF DARK_BACKGROUND_PRIMARY = RGB(25, 25, 25); // Ultra dark
    const COLORREF DARK_BACKGROUND_SECONDARY = RGB(32, 32, 32); // Slightly lighter
    const COLORREF DARK_PANEL_BACKGROUND = RGB(40, 40, 40);   // Panel background
    const COLORREF DARK_CARD_BACKGROUND = RGB(45, 45, 45);    // Card background
    const COLORREF DARK_RIBBON_BACKGROUND = RGB(30, 30, 30);  // Ribbon background
    
    // Accent Colors - Professional palette
    const COLORREF ACCENT_PRIMARY = RGB(0, 120, 212);         // Microsoft blue
    const COLORREF ACCENT_SECONDARY = RGB(16, 124, 16);       // Success green
    const COLORREF ACCENT_WARNING = RGB(255, 140, 0);         // Warning orange
    const COLORREF ACCENT_ERROR = RGB(232, 17, 35);           // Error red
    const COLORREF ACCENT_PURPLE = RGB(162, 0, 255);          // Purple accent
    
    // Border Colors
    const COLORREF BORDER_PRIMARY = RGB(229, 229, 229);       // Light borders
    const COLORREF BORDER_SECONDARY = RGB(200, 200, 200);     // Secondary borders
    const COLORREF BORDER_FOCUS = RGB(0, 120, 212);           // Focus border
    const COLORREF DARK_BORDER_PRIMARY = RGB(62, 62, 66);     // Dark mode borders
    const COLORREF DARK_BORDER_SECONDARY = RGB(80, 80, 80);   // Dark secondary borders
    
    // Text Colors - Professional contrast
    const COLORREF TEXT_PRIMARY = RGB(0, 0, 0);               // Pure black text
    const COLORREF TEXT_SECONDARY = RGB(96, 96, 96);          // Secondary text
    const COLORREF TEXT_TERTIARY = RGB(128, 128, 128);        // Tertiary text
    const COLORREF TEXT_DISABLED = RGB(160, 160, 160);        // Disabled text
    const COLORREF DARK_TEXT_PRIMARY = RGB(255, 255, 255);    // Dark mode text
    const COLORREF DARK_TEXT_SECONDARY = RGB(204, 204, 204);  // Dark secondary text
    
    // Button States - Ultra Modern styling
    const COLORREF BUTTON_NORMAL = RGB(255, 255, 255);        // Button normal
    const COLORREF BUTTON_HOVER = RGB(248, 248, 248);         // Button hover
    const COLORREF BUTTON_PRESSED = RGB(240, 240, 240);       // Button pressed
    const COLORREF BUTTON_DISABLED = RGB(250, 250, 250);      // Button disabled
    const COLORREF DARK_BUTTON_NORMAL = RGB(50, 50, 50);      // Dark button normal
    const COLORREF DARK_BUTTON_HOVER = RGB(60, 60, 60);       // Dark button hover
    const COLORREF DARK_BUTTON_PRESSED = RGB(40, 40, 40);     // Dark button pressed
}

// Modern UI Framework - Modular Component System
class ModernDialog {
private:
    HWND dialog;
    HWND contentArea;
    HWND okButton;
    HWND cancelButton;
    RealGameAnalyzerGUI* parent;
    bool isDarkMode;
    std::string title;
    std::string content;
    int width, height;
    bool hasCancelButton;
    
public:
    ModernDialog(HWND parentWindow, RealGameAnalyzerGUI* parentApp, const std::string& title, const std::string& content, int width = 500, int height = 300, bool showCancel = false);
    ~ModernDialog();
    void centerDialog(int width, int height);
    void show();
    void hide();
    void setTitle(const std::string& newTitle);
    void setContent(const std::string& newContent);
    void refreshTheme();
    static LRESULT CALLBACK modernDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void drawDialogBackground(HDC hdc, RECT& rect, bool isDarkMode);
    static void drawModernButton(HDC hdc, RECT& rect, const std::string& text, bool isPressed, bool isHovered, bool isDarkMode);
};

class ModernButton {
private:
    HWND button;
    std::string text;
    int x, y, width, height;
    HMENU id;
    HWND parent;
    
public:
    ModernButton(HWND parentWindow, const std::string& buttonText, int posX, int posY, int w, int h, HMENU buttonId);
    HWND getHandle() const;
    void setFont(HFONT font);
    void setText(const std::string& newText);
};

class ModernLabel {
private:
    HWND label;
    std::string text;
    int x, y, width, height;
    HWND parent;
    
public:
    ModernLabel(HWND parentWindow, const std::string& labelText, int posX, int posY, int w, int h, bool center = false);
    HWND getHandle() const;
    void setFont(HFONT font);
    void setText(const std::string& newText);
};

class ModernPanel {
private:
    HWND panel;
    int x, y, width, height;
    HWND parent;
    std::string title;
    bool hasBorder;
    bool isDarkMode;
    
public:
    ModernPanel(HWND parentWindow, int posX, int posY, int w, int h, const std::string& panelTitle = "", bool border = true);
    HWND getHandle() const;
    void setBackground(HBRUSH brush);
    void setTitle(const std::string& newTitle);
    void setDarkMode(bool darkMode);
    void drawPanel(HDC hdc, RECT& rect);
};

class ModernCard {
private:
    HWND card;
    HWND titleLabel;
    HWND contentArea;
    int x, y, width, height;
    HWND parent;
    std::string title;
    std::string content;
    bool isDarkMode;
    bool isHovered;
    
public:
    ModernCard(HWND parentWindow, int posX, int posY, int w, int h, const std::string& cardTitle, const std::string& cardContent = "");
    HWND getHandle() const;
    void setTitle(const std::string& newTitle);
    void setContent(const std::string& newContent);
    void setDarkMode(bool darkMode);
    void setHovered(bool hovered);
    void drawCard(HDC hdc, RECT& rect);
};

class ModernRibbon {
private:
    HWND ribbon;
    std::vector<HWND> buttons;
    std::vector<std::string> buttonTexts;
    int x, y, width, height;
    HWND parent;
    bool isDarkMode;
    
public:
    ModernRibbon(HWND parentWindow, int posX, int posY, int w, int h);
    void addButton(const std::string& text, HMENU id);
    void setDarkMode(bool darkMode);
    void drawRibbon(HDC hdc, RECT& rect);
    HWND getHandle() const;
};

// Utility functions for modern UI
namespace ModernUI {
    void drawModernButton(RealGameAnalyzerGUI* parent, LPDRAWITEMSTRUCT lpDrawItem);
    void applyModernTheme(HWND window, RealGameAnalyzerGUI* parent);
    void createModernFonts(HFONT& modernFont, HFONT& boldFont, HFONT& headerFont);
    void updateThemeBrushes(RealGameAnalyzerGUI* parent);
    
    // Enhanced dialog utilities
    void drawDialogShadow(HDC hdc, RECT& rect, bool isDarkMode);
    void drawGradientBackground(HDC hdc, RECT& rect, COLORREF startColor, COLORREF endColor, bool isVertical = true);
    void drawRoundedRectangle(HDC hdc, RECT& rect, int radius, COLORREF fillColor, COLORREF borderColor = 0);
    void drawTextWithShadow(HDC hdc, const std::string& text, RECT& rect, UINT format, COLORREF textColor, COLORREF shadowColor);
    
    // Animation utilities
    void fadeInWindow(HWND hwnd, int duration = 200);
    void fadeOutWindow(HWND hwnd, int duration = 200);
    void slideInWindow(HWND hwnd, int direction = 0); // 0=from top, 1=from bottom, 2=from left, 3=from right
    
    // Component utilities
    void createModernLayout(HWND parent, RealGameAnalyzerGUI* app);
    void updateComponentThemes(HWND parent, RealGameAnalyzerGUI* app);
    void drawComponentBackground(HDC hdc, RECT& rect, const std::string& componentType, bool isDarkMode);
    
    // UX Consistency utilities
    void applyConsistentSpacing(HWND parent, int margin = 10, int padding = 8);
    void applyConsistentTypography(HWND parent, RealGameAnalyzerGUI* app);
    void applyConsistentColors(HWND parent, RealGameAnalyzerGUI* app);
    void enhanceAccessibility(HWND parent);
    void createVisualHierarchy(HWND parent, RealGameAnalyzerGUI* app);
}
