#include <windows.h>
#include <windowsx.h>

#include "deflib.c"
#include "types.h"
#include "win32.c"

#include "string.c"
#include "format.c"
#include "font.c"
#include "item.c"
#include "itemMovement.c"

#include "performance.c"
#include "rendering.c"
#include "undoRedo.c"

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
bool isSaved = 1;
bool isFullscreen = 0;
bool isSpecialSymbolsShown = 0;
bool isPerfShown = 0;
bool isJustSwitchedModeToInsert = 0;

Mode mode = ModeNormal;
FontData consolasFont14;
FontData segoeUiFont14;
FontData segoeUiFont14Selected;
FontData segoeUiFont14Background;
FontData segoeUiFont14SelectedInsert;
float appTimeSec = 0;

V2i screenOffset;
V2i mouse;
bool isMovingViaMouse = 0;

Arena arena;
Item root;
Item* selectedItem;

// when I handle 'i' WM_KEYDOWN, immediatelly after that WM_CHAR insert a char into the title
i32 ignoreNextCharEvent = 0;

u32 cursorPosition = 0;
i32 ctrlPressed = 0;
i32 isShiftPressed = 0;
i32 isAltPressed = 0;

ChangeHistory history;
StringBuffer insertingAtStr;

BITMAPINFO bitmapInfo;


void ChangeSelection(Item* item)
{
    selectedItem = item;
    cursorPosition = 0;
}

inline void OnKeyDown(HWND window, char key)
{
    switch (key)
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
        if (mode == ModeInsert)
        {

            mode = ModeNormal;
        }
        break;
    case 'D':
        if (mode == ModeNormal)
        {
            // how do I free memory in arena such that it is usable
            // this is a memory leak currently, but in linear arena I have no easy way to handle this now
            OnItemRemoved(&history, selectedItem);
            
            Item* prev = GetItemPrevSibling(selectedItem);
            RemoveItemFromTree(selectedItem);

            if(selectedItem->nextSibling)
                selectedItem = selectedItem->nextSibling;
            else if (prev)
                selectedItem = prev;
            else if(IsRoot(selectedItem->parent))
                selectedItem = 0;
            else 
                selectedItem = selectedItem->parent;
        }
        break;
    case 'O':
        if (mode == ModeNormal)
        {
            Item *newItem = (Item *)ArenaPush(&arena, sizeof(Item));
            newItem->title = StringBufferEmptyWithCapacity(4);
            if (!selectedItem)
                InsertItemAsFirstChild(&root, newItem);
            else
            {
                if (isShiftPressed)
                    InsertItemBefore(selectedItem, newItem);
                else if (ctrlPressed)
                    InsertItemAsFirstChild(selectedItem, newItem);
                else
                    InsertItemAfter(selectedItem, newItem);
            }

            selectedItem = newItem;

            cursorPosition = 0;
            mode = ModeInsert;

            ignoreNextCharEvent = 1;
        }
        break;
    case 'W':
        if ((mode == ModeNormal || ctrlPressed))
        {
            i32 nextWord = IndexAfter(&selectedItem->title, cursorPosition, ' ');
            if (nextWord >= 0)
                cursorPosition = nextWord + 1;
            else
                cursorPosition = selectedItem->title.size;
        }
        break;
    case 'B':
        if ((mode == ModeNormal || ctrlPressed))
        {
            i32 nextWord = IndexBefore(&selectedItem->title, cursorPosition - 1, ' ');
            if (nextWord >= 0)
                cursorPosition = nextWord + 1;
            else
                cursorPosition = 0;
        }
        break;

    case 'Y':
        if ((mode == ModeNormal || ctrlPressed) && cursorPosition > 0)
        {
            RemoveCharAt(&selectedItem->title, cursorPosition - 1);
            cursorPosition--;
        }
        break;
    case VK_BACK:
        if (cursorPosition > 0)
        {
            RemoveCharAt(&selectedItem->title, cursorPosition - 1);
            cursorPosition--;
        }
        break;
    case VK_DELETE:
        if (cursorPosition < selectedItem->title.size)
        {
            RemoveCharAt(&selectedItem->title, cursorPosition);
        }
        break;
    // case 'U':
    //     if ((mode == ModeNormal || ctrlPressed) && cursorPosition < selectedItem->title.size)
    //     {
    //         RemoveCharAt(&selectedItem->title, cursorPosition);
    //     }
    //     break;
    case 'U':
        if(mode == ModeNormal)
        {
            if(isShiftPressed)
                RedoLastChange(&history);
            else 
                UndoLastChange(&history);
        }
    break;
    case 'I':
        if (mode == ModeNormal)
        {
            mode = ModeInsert;
            ignoreNextCharEvent = 1;
            BeforeRenameItem(&history, selectedItem);
        }
        break;
    case 'E':
        if ((mode == ModeNormal || ctrlPressed) && cursorPosition > 0)
            cursorPosition--;
        break;
    case 'R':
        if ((mode == ModeNormal || ctrlPressed) && cursorPosition < selectedItem->title.size)
            cursorPosition++;
        break;
    case 'H':
        if (mode == ModeInsert)
            return;

        if(isAltPressed)
            MoveItemLeft(selectedItem);
        else 
        {
            if (selectedItem->firstChild && !selectedItem->isClosed)
                selectedItem->isClosed = 1;
            else if (selectedItem->parent->parent)
                ChangeSelection(selectedItem->parent);
        }

        break;
    case 'L':
        if (mode == ModeInsert)
            return;

        if(isAltPressed)
            MoveItemRight(selectedItem);
        else 
        {
            if (selectedItem->isClosed)
                selectedItem->isClosed = 0;
            else if (selectedItem->firstChild)
                ChangeSelection(selectedItem->firstChild);
        }
        break;
    case 'J':
        if (mode == ModeInsert)
            return;

        if (isAltPressed)
            MoveItemDown(selectedItem);
        else
        {
            if (selectedItem->firstChild && !selectedItem->isClosed)
                ChangeSelection(selectedItem->firstChild);
            else if (selectedItem->nextSibling)
                ChangeSelection(selectedItem->nextSibling);
            else
            {
                Item *parentWithSibling = selectedItem->parent;
                while (parentWithSibling && !parentWithSibling->nextSibling)
                    parentWithSibling = parentWithSibling->parent;

                if (parentWithSibling)
                    ChangeSelection(parentWithSibling->nextSibling);
            }
        }
        break;
    case 'K':
        if (mode == ModeInsert)
            return;

        if (isAltPressed)
            MoveItemUp(selectedItem);
        else
        {
            if (selectedItem->parent->firstChild == selectedItem && selectedItem->parent->parent)
            {
                ChangeSelection(selectedItem->parent);
            }
            else if (root.firstChild != selectedItem)
            {
                Item *prevSibling = selectedItem->parent->firstChild;

                while (prevSibling->nextSibling != selectedItem)
                    prevSibling = prevSibling->nextSibling;

                if (prevSibling->firstChild && !prevSibling->isClosed)
                {
                    // select most nested
                    Item *last = prevSibling->firstChild;
                    do
                    {
                        while (last->nextSibling)
                            last = last->nextSibling;

                        if (last->firstChild && !prevSibling->isClosed)
                            last = last->firstChild;
                    } while (last->firstChild || last->nextSibling);
                    ChangeSelection(last);
                }
                else
                {
                    ChangeSelection(prevSibling);
                }
            }
        }

        break;
    case 'S':
        if (ctrlPressed)
        {
            SaveStateIntoFile(&root, FILE_PATH);
            isSaved = 1;
        }
        break;
    }
}

void DrawTree(Item* root);
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
    
            workingCanvas = &canvas;

            DrawTree(&root);
            StretchDIBits(dc, 0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, canvas.pixels, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);

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
                isSaved = 0;
            }
        break;

        case WM_KEYDOWN:
        
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
        DrawBitmap(texture, x, middleY);

        if(cursorPos == i)
            PaintRect(x - 1, y - currentFont->charHeight / 2 - 3, 2, currentFont->charHeight, activeColor);

        x += texture->width + GetKerningValue(*ch, *(ch + 1));
        ch++;
        i++;
    }
    if(cursorPos == i)
        PaintRect(x - 1, y - currentFont->charHeight / 2 - 3, 2, currentFont->charHeight, activeColor);
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
        DrawBitmap(texture, x, y);
        ch++;
        x += currentFont->charWidth;
    }
}

void DrawTree(Item* root)
{
    currentFont = &segoeUiFont14;
    f32 step = PX(20);

    f32 padding = 20;

    f32 y = padding;
    f32 iconSize = 10;
    f32 iconToTextSpace = PX(6);
    f32 lineHeight = 1.3f;
    f32 squareToLine = PX(10);

    TreeIteration iteration = StartIteratingChildren(root);
    Item* current = iteration.stack[iteration.currentItem];

    while(current)
    {
        f32 x = padding + iteration.levels[iteration.currentItem] * step;
        u32 activeColor = mode == ModeNormal ?  0xaaffaa : 0xffaaaa;
        u32 rectColor = current == selectedItem ? activeColor : 0xaaaaaa;
        PaintSquareCentered(x, y, iconSize, rectColor);

        if(!current->firstChild)
        {
            f32 border = 1;
            PaintSquareCentered(x, y, iconSize - border * 2, 0x000000);
        }
        else if(!current->isClosed)
        {
            f32 lineW = 2;
            f32 childrenLineHeight = currentFont->charHeight * lineHeight * ItemGetTotalChildrenCount(current);
            PaintRect(x - lineW / 2, y + (currentFont->charHeight * lineHeight / 2), lineW, childrenLineHeight, 0x333333);
        }

        f32 textX = x + iconSize / 2 + iconToTextSpace;

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

        current = GetNextChild(&iteration);

        y += currentFont->charHeight * lineHeight;
    }

    currentFont = &segoeUiFont14Background;
    char* state = isSaved ? "Saved" : "Modified";
    DrawLabelMiddleLeft(canvas.width / 2, canvas.height - currentFont->charHeight / 2, state, -1);
}


void WinMainCRTStartup()
{
    PreventWindowsDPIScaling();
    timeBeginPeriod(1);

    arena = CreateArena(Megabytes(54));


    InitFontSystem();
    InitFont(&consolasFont14, FontInfoClearType("Consolas", 14, 0xfff0f0f0, 0x00000000), &arena);
    InitFont(&segoeUiFont14, FontInfoClearType("Segoe UI", 13, 0xfff0f0f0, 0x00000000), &arena);
    InitFont(&segoeUiFont14Selected, FontInfoClearType("Segoe UI", 13, 0xffb0ffb0, 0x00000000), &arena);
    InitFont(&segoeUiFont14SelectedInsert, FontInfoClearType("Segoe UI", 13, 0xffb0b0ff, 0x00000000), &arena);
    InitFont(&segoeUiFont14Background, FontInfoClearType("Segoe UI", 13, 0xff808080, 0x00000000), &arena);

    HWND window = OpenWindow(OnEvent, 0x222222);
    HDC dc = GetDC(window);

    MSG msg;
    InitPerf();

    StringBuffer file = ReadFileIntoDoubledSizedBuffer(FILE_PATH);
    ParseFileContent(&root, file, &arena);
    selectedItem = root.firstChild;

    while(isRunning)
    {
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {

            if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
            {
                if(msg.wParam == VK_CONTROL)
                    ctrlPressed = 1;
                else if(msg.wParam == VK_SHIFT)
                    isShiftPressed = 1;
                else if(msg.wParam == VK_MENU)
                    isAltPressed = 1;

                OnKeyDown(window, msg.wParam);
                    
                // prevent OS handling keys like ALT + J
                if (isAltPressed && msg.wParam != VK_F4)
                    continue;
            }
            if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
            {
                if(msg.wParam == VK_CONTROL)
                    ctrlPressed = 0;
                else if(msg.wParam == VK_SHIFT)
                    isShiftPressed = 0;
                else if(msg.wParam == VK_MENU)
                    isAltPressed = 0;
            }

            TranslateMessage(&msg);
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
        // Sleep(4);
        EndMetric(SleepMetric);
    }

    ExitProcess(0);
}