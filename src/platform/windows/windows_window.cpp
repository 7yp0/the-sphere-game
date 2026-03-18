#include "windows_window.h"
#include "renderer/renderer.h"
#include <windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cstring>
#include <cstdio>
#include <string>

// Helper macros for extracting coordinates from LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define GET_WHEEL_DELTA_WPARAM(wp) ((short)HIWORD(wp))

// Forward declarations
namespace Platform {

    // Letterbox-Viewport-Parameter
    static int g_letterbox_offset_x = 0;
    static int g_letterbox_offset_y = 0;
    static int g_letterbox_width = 0;
    static int g_letterbox_height = 0;

    // Setzt den OpenGL-Viewport und berechnet Letterbox-Offsets für Integer-Scaling 16:9
    void update_letterbox_viewport()
    {
        RECT clientRect = {};
        GetClientRect(g_hwnd, &clientRect);
        int winW = clientRect.right - clientRect.left;
        int winH = clientRect.bottom - clientRect.top;
        const int baseW = 320;
        const int baseH = 180;
        int scale = (int)fmin(floor((float)winW / baseW), floor((float)winH / baseH));
        if (scale < 1) scale = 1;
        int targetW = baseW * scale;
        int targetH = baseH * scale;
        int offsetX = (winW - targetW) / 2;
        int offsetY = (winH - targetH) / 2;
        g_letterbox_offset_x = offsetX;
        g_letterbox_offset_y = offsetY;
        g_letterbox_width = targetW;
        g_letterbox_height = targetH;
        glViewport(offsetX, offsetY, targetW, targetH);
        Renderer::set_viewport((uint32_t)targetW, (uint32_t)targetH);
    }
    void set_mouse_pos(Vec2 pos);
    void set_mouse_clicked(bool clicked);
    void set_key_pressed(int key_code, bool pressed);
}

namespace Platform {

static HWND g_hwnd = nullptr;
static HDC g_hdc = nullptr;
static HGLRC g_hglrc = nullptr;
static bool g_shouldClose = false;
static Vec2 g_mousePos = Vec2(0.0f, 0.0f);
static bool g_mouseClicked = false;
static bool g_mouseRightClicked = false;
static bool g_mouseDown = false;
static bool g_keys[256] = {};
static uint32_t g_window_width = 0;
static uint32_t g_window_height = 0;
static float g_scroll_delta = 0.0f;

// Fullscreen state
static bool g_fullscreen = false;
static WINDOWPLACEMENT g_windowed_placement = { sizeof(g_windowed_placement) };
static LONG g_windowed_style = 0;

// Forward declarations
void toggle_fullscreen();

// Windows message callback
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
        case WM_SIZE:
            update_letterbox_viewport();
            return 0;
{
    switch (msg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
            g_shouldClose = true;
            PostQuitMessage(0);
            return 0;

        case WM_SYSKEYDOWN:
            // Handle ALT+ENTER for fullscreen toggle
            if (wparam == VK_RETURN && (lparam & (1 << 29)))  // ALT is held (bit 29)
            {
                toggle_fullscreen();
                return 0;
            }
            return DefWindowProcW(hwnd, msg, wparam, lparam);

        case WM_KEYDOWN:
            set_key_pressed((int)wparam, true);
            return 0;

        case WM_KEYUP:
            set_key_pressed((int)wparam, false);
            return 0;

        case WM_LBUTTONDOWN:
            set_mouse_clicked(true);
            set_mouse_down(true);
            return 0;

        case WM_LBUTTONUP:
            set_mouse_down(false);
            return 0;

        case WM_RBUTTONDOWN:
            set_mouse_right_clicked(true);
            return 0;

        case WM_MOUSEMOVE:
        {
            int x = GET_X_LPARAM(lparam);
            int y = GET_Y_LPARAM(lparam);
            // Letterbox: Nur Spielfeldbereich berücksichtigen
            int relX = x - g_letterbox_offset_x;
            int relY = y - g_letterbox_offset_y;
            // Clamp auf Spielfeld
            if (relX < 0) relX = 0;
            if (relY < 0) relY = 0;
            if (relX >= g_letterbox_width) relX = g_letterbox_width - 1;
            if (relY >= g_letterbox_height) relY = g_letterbox_height - 1;
            // Y-Flip: OpenGL und Spiel erwarten Y=0 unten
            int flippedY = g_letterbox_height - 1 - relY;
            set_mouse_pos(Vec2((float)relX, (float)flippedY));
            return 0;
        }

        case WM_MOUSEWHEEL:
        {
            short wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
            // Positive = scroll up (objects move up), Negative = scroll down (objects move down)
            // Each "wheel notch" is WHEEL_DELTA units (typically 120)
            g_scroll_delta += wheelDelta / 120.0f;  // Normalize to units
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

void set_key_pressed(int key_code, bool pressed)
{
    if (key_code < 256) {
        g_keys[key_code] = pressed;
    }
}

bool init_window(const WindowConfig& config)
    update_letterbox_viewport();
{
    g_window_width = config.width;
    g_window_height = config.height;

    // Convert title to wide char for Windows API
    int titleLen = std::strlen(config.title);
    wchar_t wideTitle[256];
    MultiByteToWideChar(CP_UTF8, 0, config.title, titleLen, wideTitle, 256);
    wideTitle[titleLen] = L'\0';

    // Register window class
    const wchar_t CLASS_NAME[] = L"TheSpherGameWindowClass";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(nullptr, L"Failed to register window class", L"Error", MB_OK);
        return false;
    }

    // Calculate window size needed to get desired client area
    // AdjustWindowRectEx accounts for title bar, borders, etc.
    DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT windowRect = { 0, 0, (LONG)config.width, (LONG)config.height };
    AdjustWindowRectEx(&windowRect, windowStyle, FALSE, 0);
    int adjustedWidth = windowRect.right - windowRect.left;
    int adjustedHeight = windowRect.bottom - windowRect.top;

    // Create window with adjusted size
    g_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        wideTitle,
        windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        adjustedWidth, adjustedHeight,
        nullptr, nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (!g_hwnd)
    {
        MessageBoxW(nullptr, L"Failed to create window", L"Error", MB_OK);
        return false;
    }

    // Get ACTUAL client rect (the drawable area, excluding frame/title)
    RECT clientRect = {};
    GetClientRect(g_hwnd, &clientRect);
    g_window_width = clientRect.right - clientRect.left;
    g_window_height = clientRect.bottom - clientRect.top;
    
    printf("[WINDOWS] Requested: %u x %u, Actual client area: %u x %u\n",
           config.width, config.height, g_window_width, g_window_height);
    fflush(stdout);

    // Get device context
    g_hdc = GetDC(g_hwnd);
    if (!g_hdc)
    {
        MessageBoxW(nullptr, L"Failed to get device context", L"Error", MB_OK);
        DestroyWindow(g_hwnd);
        return false;
    }

    // First, set a basic pixel format to get a context for wglChoosePixelFormatARB
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(g_hdc, &pfd);
    if (!pixelFormat || !SetPixelFormat(g_hdc, pixelFormat, &pfd))
    {
        MessageBoxW(nullptr, L"Failed to set pixel format", L"Error", MB_OK);
        ReleaseDC(g_hwnd, g_hdc);
        DestroyWindow(g_hwnd);
        return false;
    }

    // Create a temporary context to get function pointers
    HGLRC tempContext = wglCreateContext(g_hdc);
    wglMakeCurrent(g_hdc, tempContext);

    // Now use wglChoosePixelFormatARB for better control (WGL_ARB_pixel_format)
    typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

    if (wglChoosePixelFormatARB)
    {
        // WGL_ARB_pixel_format attribute constants
        const int WGL_DRAW_TO_WINDOW_ARB = 0x2001;
        const int WGL_SUPPORT_OPENGL_ARB = 0x2010;
        const int WGL_DOUBLE_BUFFER_ARB = 0x2011;
        const int WGL_PIXEL_TYPE_ARB = 0x2013;
        const int WGL_TYPE_RGBA_ARB = 0x202B;
        const int WGL_COLOR_BITS_ARB = 0x2014;
        const int WGL_ALPHA_BITS_ARB = 0x201B;
        const int WGL_DEPTH_BITS_ARB = 0x2022;

        int attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, 1,
            WGL_SUPPORT_OPENGL_ARB, 1,
            WGL_DOUBLE_BUFFER_ARB, 1,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_ALPHA_BITS_ARB, 8,        // CRITICAL: Request alpha bits
            WGL_DEPTH_BITS_ARB, 24,
            0
        };

        int pixelFormats[1];
        UINT numFormats;
        if (wglChoosePixelFormatARB(g_hdc, attribs, nullptr, 1, pixelFormats, &numFormats) && numFormats > 0)
        {
            pixelFormat = pixelFormats[0];
            SetPixelFormat(g_hdc, pixelFormat, &pfd);
        }
    }

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(tempContext);

    // Create OpenGL rendering context
    g_hglrc = wglCreateContext(g_hdc);
    if (!g_hglrc)
    {
        MessageBoxW(nullptr, L"Failed to create OpenGL context", L"Error", MB_OK);
        ReleaseDC(g_hwnd, g_hdc);
        DestroyWindow(g_hwnd);
        return false;
    }

    // Make the context current
    if (!wglMakeCurrent(g_hdc, g_hglrc))
    {
        MessageBoxW(nullptr, L"Failed to make OpenGL context current", L"Error", MB_OK);
        wglDeleteContext(g_hglrc);
        ReleaseDC(g_hwnd, g_hdc);
        DestroyWindow(g_hwnd);
        return false;
    }

    // Initialize GLEW to load OpenGL extensions
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        const wchar_t* error = reinterpret_cast<const wchar_t*>(glewGetErrorString(err));
        MessageBoxW(nullptr, (std::wstring(L"GLEW initialization failed: ") + error).c_str(), L"Error", MB_OK);
        wglDeleteContext(g_hglrc);
        ReleaseDC(g_hwnd, g_hdc);
        DestroyWindow(g_hwnd);
        return false;
    }

    // Enable vsync (swap interval)
    // wglSwapIntervalEXT might not be available, so we check
    typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    
    if (wglSwapIntervalEXT)
    {
        wglSwapIntervalEXT(1);  // 1 = vsync enabled
    }

    ShowWindow(g_hwnd, SW_SHOW);
    SetForegroundWindow(g_hwnd);
    SetFocus(g_hwnd);

    return true;
}

void swap_buffers()
{
    if (g_hdc && g_hglrc)
    {
        wglMakeCurrent(g_hdc, g_hglrc);
        update_letterbox_viewport();
        SwapBuffers(g_hdc);
    }
}

bool window_should_close()
{
    MSG msg = {};
    
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return g_shouldClose;
}

void request_quit()
{
    g_shouldClose = true;
}

void shutdown()
{
    if (g_hglrc)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_hglrc);
        g_hglrc = nullptr;
    }

    if (g_hdc)
    {
        ReleaseDC(g_hwnd, g_hdc);
        g_hdc = nullptr;
    }

    if (g_hwnd)
    {
        DestroyWindow(g_hwnd);
        g_hwnd = nullptr;
    }
}

Vec2 get_mouse_pos()
{
    return g_mousePos;
}

uint32_t get_window_height()
{
    return g_window_height;
}

uint32_t get_window_width()
{
    return g_window_width;
}

bool mouse_clicked()
{
    bool clicked = g_mouseClicked;
    g_mouseClicked = false;
    return clicked;
}

bool key_pressed(int key_code)
{
    if (key_code < 256) {
        return g_keys[key_code];
    }
    return false;
}

bool shift_down()
{
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
}

bool control_down()
{
    return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
}

float scroll_delta()
{
    float delta = g_scroll_delta;
    g_scroll_delta = 0.0f;
    return delta;
}

void set_mouse_pos(Vec2 pos)
{
    g_mousePos = pos;
}

void set_mouse_clicked(bool clicked)
{
    g_mouseClicked = clicked;
}

bool mouse_down()
{
    return g_mouseDown;
}

void set_mouse_down(bool down)
{
    g_mouseDown = down;
}

bool mouse_right_clicked()
{
    bool clicked = g_mouseRightClicked;
    g_mouseRightClicked = false;
    return clicked;
}

void set_mouse_right_clicked(bool clicked)
{
    g_mouseRightClicked = clicked;
}

void show_system_cursor(bool show)
{
    ShowCursor(show ? TRUE : FALSE);
}

void toggle_fullscreen()
{
    if (!g_fullscreen)
    {
        // Save current window state before going fullscreen
        g_windowed_style = GetWindowLong(g_hwnd, GWL_STYLE);
        GetWindowPlacement(g_hwnd, &g_windowed_placement);
        
        // Get monitor info for the monitor the window is on
        HMONITOR hmon = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hmon, &mi);
        
        // Borderless fullscreen: cover the ENTIRE monitor
        // The renderer will upscale from 320x180 FBO to this size
        int monitor_width = mi.rcMonitor.right - mi.rcMonitor.left;
        int monitor_height = mi.rcMonitor.bottom - mi.rcMonitor.top;
        
        // Remove window decorations and cover full monitor
        SetWindowLong(g_hwnd, GWL_STYLE, g_windowed_style & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowPos(g_hwnd, HWND_TOP,
                     mi.rcMonitor.left, mi.rcMonitor.top,
                     monitor_width, monitor_height,
                     SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
        
        // Update to full monitor size - renderer handles scaling
        g_window_width = monitor_width;
        g_window_height = monitor_height;
        
        g_fullscreen = true;
        update_letterbox_viewport();
    }
    else
    {
        // Restore windowed mode
        SetWindowLong(g_hwnd, GWL_STYLE, g_windowed_style);
        SetWindowPlacement(g_hwnd, &g_windowed_placement);
        SetWindowPos(g_hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
        
        // Update window dimensions from restored window
        RECT clientRect = {};
        GetClientRect(g_hwnd, &clientRect);
        g_window_width = clientRect.right - clientRect.left;
        g_window_height = clientRect.bottom - clientRect.top;
        
        g_fullscreen = false;
        update_letterbox_viewport();
    }
    
    printf("[WINDOWS] Fullscreen: %s, Size: %u x %u\n", 
           g_fullscreen ? "ON" : "OFF", g_window_width, g_window_height);
    fflush(stdout);
}

bool is_fullscreen()
{
    return g_fullscreen;
}

bool alt_down()
{
    return (GetKeyState(VK_MENU) & 0x8000) != 0;
}

}  // namespace Platform
