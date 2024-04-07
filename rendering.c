#pragma once
#include "types.h"

i32 Round(f32 v)
{
    return (i32)(v + 0.5f);
}

MyBitmap *workingCanvas;

inline void PaintRect(f32 x, f32 y, f32 width, f32 height, u32 color)
{
    i32 left = Round(x);
    i32 top = Round(y);
    i32 right = Round(x + width);
    i32 bottom = Round(y + height);

    if (right < 0 || left > workingCanvas->width)
        return;

    if (left < 0)
        left = 0;

    if (right > workingCanvas->width)
        right = workingCanvas->width;

    if (bottom < 0 || top > workingCanvas->height)
        return;

    if (top < 0)
        top = 0;

    if (bottom > workingCanvas->height)
        bottom = workingCanvas->height;

    for (i32 y = top; y < bottom; y += 1)
    {
        for (i32 x = left; x < right; x += 1)
        {
            *(workingCanvas->pixels + y * workingCanvas->width + x) = color;
        }
    }
}

inline void PaintSquareCentered(f32 x, f32 y, f32 size, u32 color)
{
    PaintRect(x - size / 2, y - size / 2, size, size, color);
}

inline void DrawBitmap(MyBitmap *bitmapToDraw, f32 x, f32 y)
{
    i32 left = Round(x);
    i32 top = Round(y);

    i32 widthToDraw = bitmapToDraw->width;
    i32 heightToDraw = bitmapToDraw->height;

    i32 right = left + widthToDraw;
    i32 bottom = top + heightToDraw;

    u32 *source = (u32 *)bitmapToDraw->pixels;

    if (right < 0 || left > workingCanvas->width)
        return;

    if (left < 0)
    {
        widthToDraw -= -left;
        source += -left;
        left = 0;
    }

    if (right > workingCanvas->width)
        return;

    if (bottom < 0 || top > workingCanvas->height)
        return;

    if (top < 0)
    {
        heightToDraw -= -top;
        source += -top * bitmapToDraw->width;
        top = 0;
    }

    if (bottom > workingCanvas->height)
        return;

    u32 *row = (u32 *)workingCanvas->pixels + left + top * workingCanvas->width;

    for (u32 y = 0; y < heightToDraw; y += 1)
    {
        u32 *pixel = row;
        u32 *sourcePixel = source;
        for (u32 x = 0; x < widthToDraw; x += 1)
        {
            *pixel = *sourcePixel;
            sourcePixel += 1;
            pixel += 1;
        }
        source += bitmapToDraw->width;
        row += workingCanvas->width;
    }
}

