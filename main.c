#include <windows.h>
#include <windowsx.h>

#include "deflib.c"
#include "types.h"
#include "win32.c"

#include "string.c"
#include "format.c"
#include "font.c"
#include "item.c"

#include "performance.c"

#define FILE_PATH "../tasks.txt"

typedef enum Mode
{
    ModeNormal,
    ModeInsert
} Mode;

f32 appScale;
#define PX(val) ((val) *appScale)

u32 screenWidth;
u32 screenHeight;
MyBitmap canvas;
bool isRunning = 1;
bool isFullscreen = 0;
bool isSpecialSymbolsShown = 0;
bool isJustSwitchedModeToInsert = 0;

Mode mode = ModeNormal;
FontData consolasFont14;
FontData segoeUiFont14;
float appTimeSec = 0;

V2i screenOffset;
V2i mouse;
bool isMovingViaMouse = 0;

BITMAPINFO bitmapInfo;

inline void CopyBitmapRectTo(MyBitmap *sourceT, MyBitmap *destination, u32 offsetX, u32 offsetY)
{
    u32 *row = (u32 *)destination->pixels + destination->width * (destination->height - 1) + offsetX - offsetY * destination->width;
    u32 *source = (u32 *)sourceT->pixels + sourceT->width * (sourceT->height - 1);
    for (u32 y = 0; y < sourceT->height; y += 1)
    {
        u32 *pixel = row;
        u32 *sourcePixel = source;
        for (u32 x = 0; x < sourceT->width; x += 1)
        {
            //stupid fucking logic needs to replaced
            if(*sourcePixel != 0xff000000 && (y + offsetY) > 0 && (x + offsetX) > 0 && y + offsetY < destination->height && x + offsetX < destination->width)
                *pixel = *sourcePixel;
            sourcePixel += 1;
            pixel += 1;
            
        }
        source -= sourceT->width;
        row -= destination->width;
    }
}

inline void PaintRect(MyBitmap *destination, i32 offsetX, i32 offsetY, u32 width, u32 height, u32 color)
{
    // need to check bounds of the screen
    u32 *row = (u32 *)destination->pixels + destination->width * (destination->height - 1) + offsetX - offsetY * destination->width;
    for (i32 y = 0; y < height; y += 1)
    {
        u32 *pixel = row;
        for (i32 x = 0; x < width; x += 1)
        {
            if((y + offsetY) > 0 && (x + offsetX) > 0 && y + offsetY < destination->height && x + offsetX < destination->width)
                *pixel = color;
            pixel += 1;
            
        }
        row -= destination->width;
    }
}



// void InsertChartUnderCursor(StringBuffer* buffer, WPARAM ch)
// {
//     WPARAM code = ch == '\r' ? '\n' : ch;
//     InsertCharAt(buffer, cursor.pos, code);
//     cursor.pos++;
//     // UpdateCursorPosition(buffer, cursor.cursorIndex + 1);

//     // cursor.selectionStart = SELECTION_NONE;
// }

LRESULT OnEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message){
        case WM_QUIT:
        case WM_DESTROY:
            PostQuitMessage(0);
            isRunning = 0;
        break;

        case WM_PAINT:
            PAINTSTRUCT paint;
            BeginPaint(window, &paint);
            EndPaint(window, &paint);
        break;

        case WM_LBUTTONDOWN: 
            isMovingViaMouse = 1;
        break;

        case WM_LBUTTONUP: 
            isMovingViaMouse = 0;
        break;

        case WM_SIZE:
            HDC dc = GetDC(window);
            appScale = (float)GetDeviceCaps(dc, LOGPIXELSY) / (float)USER_DEFAULT_SCREEN_DPI;
            screenWidth = LOWORD(lParam);
            screenHeight = HIWORD(lParam);
            InitBitmapInfo(&bitmapInfo, screenWidth, screenHeight);
            canvas.width =  screenWidth;
            canvas.height = screenHeight;
            canvas.bytesPerPixel = 4;
            //TODO: Initialize Arena of screen size and assign proper width and height on resize
            if(canvas.pixels)
                VirtualFreeMemory(canvas.pixels);
            canvas.pixels = VirtualAllocateMemory(sizeof(u32) * screenWidth * screenHeight);
        break;

        case WM_KEYDOWN:
            switch(wParam)
            {
                case VK_F11: 
                    isFullscreen = !isFullscreen;
                    SetFullscreen(window, isFullscreen);
                break;
            }
        break; 
    }
    return DefWindowProc(window, message, wParam, lParam);
}

void DrawLabelMiddleLeft(i32 x, i32 y, char* label)
{
    char* ch = label;
    i32 middleY = y - currentFont->charHeight / 2 - 3; // - 1 because for the better visual quality, but will break for bigger fontsize
    while (*ch)
    {
        MyBitmap *texture = &currentFont->textures[*ch];
        CopyBitmapRectTo(texture, &canvas, x, middleY);
        x += texture->width + GetKerningValue(*ch, *(ch + 1));
        ch++;
    }
}

void ReportAt(i32 rowIndex, char* label, u32 val, char* metric)
{
    char buff[40] = {0};
    u32 buffIndex = 0;
    while (*label)
    {
        buff[buffIndex++] = *label;
        label++;
    }
    buff[buffIndex++] = ':';
    buff[buffIndex++] = ' ';

    i32 symbols = FormatNumber(val, buff + buffIndex);
    i32 x = screenWidth - currentFont->charWidth * (symbols + buffIndex + 2);
    i32 y = screenHeight - currentFont->charHeight * (rowIndex + 1);

    while(*metric)
    {
        buff[symbols + buffIndex] = *metric;
        metric++;
        buffIndex++;
    }
    buff[symbols + buffIndex] = '\0';

    char *ch = buff;
    while (*ch)
    {
        MyBitmap *texture = &currentFont->textures[*ch];
        CopyBitmapRectTo(texture, &canvas, x, y);
        ch++;
        x += currentFont->charWidth;
    }
}

void UpdateAndDraw(Item* root)
{
    currentFont = &segoeUiFont14;
    i32 step = PX(20);
    i32 padding = PX(30);
    i32 y = padding;
    i32 iconSize = PX(6);
    i32 iconToTextSpace = PX(6);
    f32 lineHeight = 1.3f;
    i32 squareToLine = PX(10);

    Item* current = root->firstChild;
    Item* stack[256] = {root};
    i32 currentItem = 0;

    while(current)
    {
        i32 x = padding + currentItem * step;
        PaintRect(&canvas, x - iconSize / 2, y - iconSize / 2, iconSize, iconSize, 0xaaaaaa);
        i32 textX = x + iconSize / 2 + iconToTextSpace;

        DrawLabelMiddleLeft(textX, y, current->title);

        if(current->firstChild)
        {
            i32 childrenCount = 1; //ItemGetTotalChildrenCount(current);
            
            PaintRect(&canvas, x - 1, y + iconSize / 2 + squareToLine, PX(2), currentFont->charHeight * lineHeight * childrenCount, 0x333333);

            stack[++currentItem] = current;
            current = current->firstChild;
        } else if (current->nextSibling)
        {
            current = current->nextSibling;
        } else 
        {
            Item* itemInStack = stack[currentItem--]->nextSibling;
            while(!itemInStack && currentItem >= 0)
                itemInStack = stack[currentItem--]->nextSibling;
            
            current = itemInStack;
        }

        y += currentFont->charHeight * lineHeight;
    }

}

void WinMainCRTStartup()
{
    PreventWindowsDPIScaling();
    timeBeginPeriod(1);

    HWND window = OpenWindow(OnEvent, 0x222222);
    HDC dc = GetDC(window);
    Arena arena = CreateArena(Megabytes(54));

    InitFontSystem();
    InitFont(&consolasFont14, FontInfoClearType("Consolas", 14, 0xfff0f0f0, 0x00000000), &arena);
    InitFont(&segoeUiFont14, FontInfoClearType("Segoe UI", 13, 0xfff0f0f0, 0x00000000), &arena);

    MSG msg;
    InitPerf();

    Item root = {0};

    FileContent file = ReadMyFileImp(FILE_PATH);
    ParseFileContent(&root, file, &arena);

    while(isRunning)
    {
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        StartMetric(Memory);
        memset(canvas.pixels, 0, canvas.bytesPerPixel * canvas.width * canvas.height);
        u32 usMemory = EndMetric(Memory);

        StartMetric(Draw);
        UpdateAndDraw(&root);
        u32 usDraw = EndMetric(Draw);

        u32 usPerFrame = EndMetric(Overall);

        currentFont = &consolasFont14;
        ReportAt(4, "Memory", usMemory, "us");
        ReportAt(3, "Drawing", usDraw, "us");
        ReportAt(2, "DiBits", GetMicrosecondsFor(DiBits), "us");
        ReportAt(1, "Sleep", GetMicrosecondsFor(SleepMetric), "us");
        ReportAt(0, "Overall", usPerFrame, "us");

        StartMetric(DiBits);
        StretchDIBits(dc, 0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, canvas.pixels, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
        EndMetric(DiBits);


        appTimeSec += ((f32)usPerFrame) / 1000.0f / 1000.0f;
        EndFrame();


        //TODO: proper sleep timing, currently just burning the CPU, dont forget about timeBeginPeriod
        StartMetric(SleepMetric);
        Sleep(10);
        EndMetric(SleepMetric);
    }

    ExitProcess(0);
}