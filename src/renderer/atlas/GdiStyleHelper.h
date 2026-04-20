// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <windows.h>
#include <til/rect.h>

namespace Microsoft::Console::Render::Atlas
{
    // GDI-style window helper class
    // This class provides utility methods to 100% replicate GDI window style
    class GdiStyleHelper
    {
    public:
        // ==================== Window Class Style ====================
        
        // GDI window class name (matches console window exactly)
        static constexpr const wchar_t* GdiWindowClassName = L"ConsoleWindowClass";
        
        // GDI window class style flags
        static constexpr UINT GetGdiClassStyle() noexcept
        {
            return CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
        }
        
        // GDI window extra space (for storing custom data)
        static constexpr int GdiWndExtraSize = 3 * sizeof(DWORD);
        
        // ==================== Window Style Flags ====================
        
        // GDI standard window style
        static constexpr DWORD GetGdiWindowStyle() noexcept
        {
            return WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL;
        }
        
        // GDI extended window style
        static constexpr DWORD GetGdiWindowExStyle() noexcept
        {
            return WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_LAYERED;
        }
        
        // ==================== Window Background Color Management ====================
        
        // GDI background color index (matches GWL_CONSOLE_BKCOLOR)
        static constexpr int GdiBackgroundColorIndex = -100;
        
        // Set GDI-style background color
        static void SetBackgroundColor(HWND hwnd, COLORREF color) noexcept
        {
            if (hwnd)
            {
                SetWindowLongW(hwnd, GdiBackgroundColorIndex, static_cast<LONG>(color));
            }
        }
        
        // Get GDI-style background color
        static COLORREF GetBackgroundColor(HWND hwnd) noexcept
        {
            if (hwnd)
            {
                return static_cast<COLORREF>(GetWindowLongW(hwnd, GdiBackgroundColorIndex));
            }
            return RGB(0, 0, 0); // Default black
        }
        
        // ==================== Scrollbar Management ====================
        
        // Set GDI-style scrollbars
        static void SetupScrollbars(HWND hwnd, int scrollMax, int scrollPage, int scrollPos = 0) noexcept
        {
            if (!hwnd) return;
            
            SCROLLINFO si = { 0 };
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_ALL;
            si.nMin = 0;
            si.nMax = scrollMax;
            si.nPage = scrollPage;
            si.nPos = scrollPos;
            
            SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
            
            // GDI always shows scrollbars
            ShowScrollBar(hwnd, SB_HORZ, TRUE);
            ShowScrollBar(hwnd, SB_VERT, TRUE);
        }
        
        // Update scrollbar position
        static void UpdateScrollbarPosition(HWND hwnd, int horzPos, int vertPos) noexcept
        {
            if (!hwnd) return;
            
            SCROLLINFO si = { 0 };
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_POS;
            
            si.nPos = horzPos;
            SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
            
            si.nPos = vertPos;
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        }
        
        // ==================== Fullscreen Mode Toggle ====================
        
        // Toggle GDI-style fullscreen mode
        static void SetFullscreen(HWND hwnd, bool fullscreen) noexcept
        {
            if (!hwnd) return;
            
            LONG style = GetWindowLongW(hwnd, GWL_STYLE);
            LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
            
            if (fullscreen)
            {
                // Remove WS_OVERLAPPEDWINDOW, add WS_POPUP
                style &= ~WS_OVERLAPPEDWINDOW;
                style |= WS_POPUP;
                
                // Remove 3D border
                exStyle &= ~WS_EX_WINDOWEDGE;
            }
            else
            {
                // Restore WS_OVERLAPPEDWINDOW
                style |= WS_OVERLAPPEDWINDOW;
                style &= ~WS_POPUP;
                
                // Restore 3D border
                exStyle |= WS_EX_WINDOWEDGE;
            }
            
            SetWindowLongW(hwnd, GWL_STYLE, style);
            SetWindowLongW(hwnd, GWL_EXSTYLE, exStyle);
            
            // Refresh window frame
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        }
        
        // ==================== Window Registration ====================
        
        // Register GDI-style window class
        static ATOM RegisterGdiWindowClass(WNDPROC windowProc, HINSTANCE hInstance, HICON hIcon, HICON hIconSm) noexcept
        {
            WNDCLASSEX wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.style = GetGdiClassStyle();
            wc.lpfnWndProc = windowProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = GdiWndExtraSize;
            wc.hInstance = hInstance;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr; // Disable automatic background painting (prevent flickering)
            wc.lpszMenuName = nullptr;
            wc.lpszClassName = GdiWindowClassName;
            wc.hIcon = hIcon;
            wc.hIconSm = hIconSm;
            
            return RegisterClassExW(&wc);
        }
        
        // ==================== Window Creation ====================
        
        // Create GDI-style window
        static HWND CreateGdiWindow(
            WNDPROC windowProc,
            HINSTANCE hInstance,
            HICON hIcon,
            HICON hIconSm,
            const wchar_t* title = L"OpenConsole",
            int x = CW_USEDEFAULT,
            int y = CW_USEDEFAULT,
            int width = CW_USEDEFAULT,
            int height = CW_USEDEFAULT,
            HWND parent = nullptr) noexcept
        {
            // Register window class
            RegisterGdiWindowClass(windowProc, hInstance, hIcon, hIconSm);
            
            // Create window
            return CreateWindowExW(
                GetGdiWindowExStyle(),
                GdiWindowClassName,
                title,
                GetGdiWindowStyle(),
                x, y, width, height,
                parent,
                nullptr,
                hInstance,
                nullptr
            );
        }
        
        // ==================== Utility Methods ====================
        
        // Check if window has GDI style
        static bool IsGdiStyleWindow(HWND hwnd) noexcept
        {
            if (!hwnd) return false;
            
            const LONG style = GetWindowLongW(hwnd, GWL_STYLE);
            const LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
            
            // Check for GDI window characteristics
            const bool hasOverlappedWindow = (style & WS_OVERLAPPEDWINDOW) != 0;
            const bool hasWindowEdge = (exStyle & WS_EX_WINDOWEDGE) != 0;
            const bool hasScrollbars = (style & WS_HSCROLL) != 0 && (style & WS_VSCROLL) != 0;
            
            return hasOverlappedWindow && hasWindowEdge && hasScrollbars;
        }
        
        // Apply GDI style to existing window
        static void ApplyGdiStyle(HWND hwnd) noexcept
        {
            if (!hwnd) return;
            
            LONG style = GetWindowLongW(hwnd, GWL_STYLE);
            LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
            
            // Add GDI style flags
            style |= GetGdiWindowStyle();
            exStyle |= GetGdiWindowExStyle();
            
            SetWindowLongW(hwnd, GWL_STYLE, style);
            SetWindowLongW(hwnd, GWL_EXSTYLE, exStyle);
            
            // Set default background color (black)
            SetBackgroundColor(hwnd, RGB(0, 0, 0));
            
            // Refresh window
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        }
    };
}
