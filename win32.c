#pragma once 

#include <windows.h>
#include <dwmapi.h>
#include "types.h"

void InitBitmapInfo(BITMAPINFO * bitmapInfo, u32 width, u32 height)
{
    bitmapInfo->bmiHeader.biSize = sizeof(bitmapInfo->bmiHeader);
    bitmapInfo->bmiHeader.biBitCount = 32;
    bitmapInfo->bmiHeader.biWidth = width;
    bitmapInfo->bmiHeader.biHeight = height; // makes rows go up, instead of going down by default
    bitmapInfo->bmiHeader.biPlanes = 1;
    bitmapInfo->bmiHeader.biCompression = BI_RGB;
}

HWND OpenWindow(WNDPROC OnEvent, u32 color)
{
    HINSTANCE instance = GetModuleHandle(0);
    WNDCLASSW windowClass = {0};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = OnEvent;
    windowClass.lpszClassName = L"MyWindow";
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.hbrBackground = CreateSolidBrush(color); // fixes a flash of a white background for a couple of frames during start
    
    RegisterClassW(&windowClass);

    HWND window = CreateWindowW(windowClass.lpszClassName, (wchar_t *)"Editor", 
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
                                0, 0, instance, 0);

    BOOL USE_DARK_MODE = TRUE;
    BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
        window, DWMWA_USE_IMMERSIVE_DARK_MODE, &USE_DARK_MODE, sizeof(USE_DARK_MODE)));

    ShowWindow(window, SW_SHOW);
    return window;
}

// DPI Scaling
// user32.dll is linked statically, so dynamic linking won't load that dll again
// taken from https://github.com/cmuratori/refterm/blob/main/refterm.c#L80
// this is done because GDI font drawing is ugly and unclear when DPI scaling is enabled

typedef BOOL WINAPI set_process_dpi_aware(void);
typedef BOOL WINAPI set_process_dpi_awareness_context(DPI_AWARENESS_CONTEXT);
static void PreventWindowsDPIScaling()
{
    HMODULE WinUser = LoadLibraryW(L"user32.dll");
    set_process_dpi_awareness_context *SetProcessDPIAwarenessContext = (set_process_dpi_awareness_context *)GetProcAddress(WinUser, "SetProcessDPIAwarenessContext");
    if (SetProcessDPIAwarenessContext)
    {
        SetProcessDPIAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }
    else
    {
        set_process_dpi_aware *SetProcessDPIAware = (set_process_dpi_aware *)GetProcAddress(WinUser, "SetProcessDPIAware");
        if (SetProcessDPIAware)
        {
            SetProcessDPIAware();
        }
    }
}


// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
WINDOWPLACEMENT prevWindowDimensions = {sizeof(prevWindowDimensions)};
void SetFullscreen(HWND window, i32 isFullscreen)
{
    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (isFullscreen)
    {
        MONITORINFO monitorInfo = {sizeof(monitorInfo)};
        if (GetWindowPlacement(window, &prevWindowDimensions) &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
        {
            SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);

            SetWindowPos(window, HWND_TOP,
                         monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                         monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &prevWindowDimensions);
        SetWindowPos(window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}



//
// File IO
//

typedef struct FileContent
{
    char *content;
    i32 size;
} FileContent;


FileContent ReadMyFileImp(char* path)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    LARGE_INTEGER size;
    GetFileSizeEx(file, &size);

    u32 fileSize = (u32)size.QuadPart;

    void *buffer = VirtualAllocateMemory(fileSize);

    DWORD bytesRead;
    ReadFile(file, buffer, fileSize, &bytesRead, 0);
    CloseHandle(file);

    FileContent res = {0};
    res.content = (char*) buffer;
    res.size = bytesRead;
    return res;
}

u32 GetMyFileSize(char *path)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    LARGE_INTEGER size;
    GetFileSizeEx(file, &size);

    CloseHandle(file);
    return (u32)size.QuadPart;
}

void ReadFileInto(char *path, u32 fileSize, char* buffer)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD bytesRead;
    ReadFile(file, buffer, fileSize, &bytesRead, 0);
    CloseHandle(file);
}