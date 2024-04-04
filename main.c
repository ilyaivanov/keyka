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
bool isPerfShown = 0;
bool isJustSwitchedModeToInsert = 0;

Mode mode = ModeNormal;
FontData consolasFont14;
FontData segoeUiFont14;
FontData segoeUiFont14Selected;
FontData segoeUiFont14SelectedInsert;
float appTimeSec = 0;

V2i screenOffset;
V2i mouse;
bool isMovingViaMouse = 0;

Item root;
Item* selectedItem;

// when I handle 'i' WM_KEYDOWN, immediatelly after that WM_CHAR insert a char into the title
i32 ignoreNextCharEvent = 0;

u32 cursorPosition = 0;
i32 ctrlPressed = 0;

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


void ChangeSelection(Item* item)
{
    selectedItem = item;
    cursorPosition = 0;
}

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

        case WM_CHAR: 
            if(ignoreNextCharEvent)
            {
                ignoreNextCharEvent = 0;
            }
            else if(mode == ModeInsert && wParam > 31)
            {
                InsertCharAt(&selectedItem->title, cursorPosition, wParam);
                cursorPosition++;
            }
        break;

        case WM_KEYDOWN:
        
            switch(wParam)
            {
                case VK_F11: 
                    isFullscreen = !isFullscreen;
                    SetFullscreen(window, isFullscreen);
                break;
                case VK_F9: 
                    isPerfShown = !isPerfShown;
                break;
                case VK_ESCAPE: 
                case VK_RETURN: 
                    if(mode == ModeInsert)
                        mode = ModeNormal;
                break;
                case 'W': 
                    if((mode == ModeNormal || ctrlPressed))
                    {
                        i32 nextWord = IndexAfter(&selectedItem->title, cursorPosition, ' ');
                        if(nextWord >= 0)
                            cursorPosition = nextWord + 1;
                        else 
                            cursorPosition = selectedItem->title.size - 1;
                    }
                break;
                case 'B': 
                    if((mode == ModeNormal || ctrlPressed))
                    {
                        i32 nextWord = IndexBefore(&selectedItem->title, cursorPosition - 1, ' ');
                        if(nextWord >= 0)
                            cursorPosition = nextWord + 1;
                        else 
                            cursorPosition = 0;
                    }
                break;

                case 'Y': 

                    if((mode == ModeNormal || ctrlPressed) && cursorPosition > 0)
                    {
                        RemoveCharAt(&selectedItem->title, cursorPosition - 1);
                        cursorPosition--;
                    }
                break;
                case 'U': 
                    if((mode == ModeNormal || ctrlPressed) && cursorPosition < selectedItem->title.size - 1)
                    {
                        RemoveCharAt(&selectedItem->title, cursorPosition);
                    }
                break;
                case 'I': 
                    if(mode == ModeNormal)
                    {
                        mode = ModeInsert;
                        ignoreNextCharEvent = 1;
                    }
                break;
                case 'E':
                    if((mode == ModeNormal || ctrlPressed) && cursorPosition > 0)
                        cursorPosition--;
                break;
                case 'R':
                    if((mode == ModeNormal || ctrlPressed) && cursorPosition < selectedItem->title.size - 2)
                        cursorPosition++;
                break;
                case 'H':
                    if(mode == ModeInsert) return DefWindowProc(window, message, wParam, lParam);

                    if(selectedItem->firstChild && !selectedItem->isClosed)
                        selectedItem->isClosed = 1;
                    else if(selectedItem->parent->parent)
                        ChangeSelection(selectedItem->parent);
                    
                break;
                case 'L': 
                    if(mode == ModeInsert) return DefWindowProc(window, message, wParam, lParam);

                    if(selectedItem->isClosed)
                        selectedItem->isClosed = 0;
                    else if(selectedItem->firstChild)
                        ChangeSelection(selectedItem->firstChild);
                break;
                case 'J':
                    if(mode == ModeInsert) return DefWindowProc(window, message, wParam, lParam);
                
                    if(selectedItem->firstChild && !selectedItem->isClosed)
                        ChangeSelection(selectedItem->firstChild);
                    else if (selectedItem->nextSibling)
                        ChangeSelection(selectedItem->nextSibling);
                    else 
                    {
                        Item* parentWithSibling = selectedItem->parent;
                        while(parentWithSibling && !parentWithSibling->nextSibling)
                            parentWithSibling = parentWithSibling->parent;
                        
                        if(parentWithSibling)
                            ChangeSelection(parentWithSibling->nextSibling);
                    }
                break;
                case 'K':
                    if(mode == ModeInsert) return DefWindowProc(window, message, wParam, lParam);

                    if(selectedItem->parent->firstChild == selectedItem && selectedItem->parent->parent)
                    {
                        ChangeSelection(selectedItem->parent);
                    }
                    else if(root.firstChild != selectedItem)
                    {
                        Item* prevSibling = selectedItem->parent->firstChild;

                        while(prevSibling->nextSibling != selectedItem)
                            prevSibling = prevSibling->nextSibling;

                        if(prevSibling->firstChild && !prevSibling->isClosed)
                        {
                            //select most nested
                            Item* last = prevSibling->firstChild;
                            do
                            {
                                while(last->nextSibling)
                                    last = last->nextSibling;

                                if(last->firstChild && !prevSibling->isClosed)
                                    last = last->firstChild;
                            }while (last->firstChild || last->nextSibling);
                            ChangeSelection(last);
                        }
                        else 
                        {
                            ChangeSelection(prevSibling);
                        }
                    }
                    
                break;
            }
        break; 
    }
    return DefWindowProc(window, message, wParam, lParam);
}

void DrawLabelMiddleLeft(i32 x, i32 y, char* label, i32 cursorPos)
{
    u32 activeColor = mode == ModeNormal ?  0xaaffaa : 0xffaaaa;
    i32 i = 0;
    char* ch = label;
    i32 middleY = y - currentFont->charHeight / 2 - 3; // - 1 because for the better visual quality, but will break for bigger fontsize
    while (*ch)
    {
        MyBitmap *texture = &currentFont->textures[*ch];
        CopyBitmapRectTo(texture, &canvas, x, middleY);

        if(cursorPos == i)
            PaintRect(&canvas, x - 1, y - currentFont->charHeight / 2 - 3, 2, currentFont->charHeight, activeColor);

        x += texture->width + GetKerningValue(*ch, *(ch + 1));
        ch++;
        i++;
    }
    if(cursorPos == i)
        PaintRect(&canvas, x - 1, y - currentFont->charHeight / 2 - 3, 2, currentFont->charHeight, activeColor);
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

void DrawTree(Item* root)
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
        u32 activeColor = mode == ModeNormal ?  0xaaffaa : 0xffaaaa;
        u32 rectColor = current == selectedItem ? activeColor : 0xaaaaaa;
        PaintRect(&canvas, x - iconSize / 2, y - iconSize / 2, iconSize, iconSize, rectColor);

        if(!current->firstChild)
        {
            f32 border = (1);
            f32 b2 = border * 2;
            PaintRect(&canvas, x - iconSize / 2 + border, y - iconSize / 2 + border, iconSize - b2, iconSize - b2, 0x000000);
        }

        i32 textX = x + iconSize / 2 + iconToTextSpace;

        if(current == selectedItem)
        {
            if (mode == ModeNormal)
                currentFont = &segoeUiFont14Selected;
            else
                currentFont = &segoeUiFont14SelectedInsert;
        }
        else 
            currentFont = &segoeUiFont14;

        DrawLabelMiddleLeft(textX, y, current->title.content, selectedItem == current ? cursorPosition : -1);

        if(current->firstChild && !current->isClosed)
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
    InitFont(&segoeUiFont14Selected, FontInfoClearType("Segoe UI", 13, 0xffb0ffb0, 0x00000000), &arena);
    InitFont(&segoeUiFont14SelectedInsert, FontInfoClearType("Segoe UI", 13, 0xffb0b0ff, 0x00000000), &arena);

    MSG msg;
    InitPerf();


    FileContent file = ReadMyFileImp(FILE_PATH);
    ParseFileContent(&root, file, &arena);
    selectedItem = root.firstChild;

    while(isRunning)
    {
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);

            if ((msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) && msg.wParam == VK_CONTROL)
            {
                ctrlPressed = 1;
            }
            if ((msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP) && msg.wParam == VK_CONTROL)
            {
                ctrlPressed = 0;
            }
            DispatchMessageA(&msg);
        }
        StartMetric(Memory);
        memset(canvas.pixels, 0, canvas.bytesPerPixel * canvas.width * canvas.height);
        u32 usMemory = EndMetric(Memory);

        StartMetric(Draw);
        DrawTree(&root);
        u32 usDraw = EndMetric(Draw);

        u32 usPerFrame = EndMetric(Overall);

        if (isPerfShown)
        {
            currentFont = &consolasFont14;
            ReportAt(4, "Memory", usMemory, "us");
            ReportAt(3, "Drawing", usDraw, "us");
            ReportAt(2, "DiBits", GetMicrosecondsFor(DiBits), "us");
            ReportAt(1, "Sleep", GetMicrosecondsFor(SleepMetric), "us");
            ReportAt(0, "Overall", usPerFrame, "us");
        }

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