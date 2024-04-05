#pragma once

#include "types.h"
#include "win32.c"


inline void PlaceLineEnd(StringBuffer *buffer)
{
    // new line doesn't count is part of the size. Not sure if this is right
    if(buffer->content)
        *(buffer->content + buffer->size) = '\0';
}

inline void MoveBytesLeft(char *ptr, int length) 
{
    for (int i = 0; i < length - 1; i++) {
        ptr[i] = ptr[i + 1];
    }
}


inline void MoveBytesRight(char *ptr, int length) 
{
    for (int i = length - 1; i > 0; i--) {
        ptr[i] = ptr[i - 1];
    }
}

inline void MoveMyMemory(char *source, char *dest, int length) 
{
    for (int i = 0; i < length; i++)
    {
        *dest = *source;
        source++;
        dest++;
    }
}

void DoubleCapacityIfFull(StringBuffer *buffer)
{
    // - 1 is because I will need to append a zero char at the end
    if(buffer->size >= buffer->capacity - 1)
    {
        char *currentStr = buffer->content;
        buffer->capacity = (buffer->capacity == 0) ? 4 : (buffer->capacity * 2);
        buffer->content = VirtualAllocateMemory(buffer->capacity);
        MoveMyMemory(currentStr, buffer->content, buffer->size);
        VirtualFreeMemory(currentStr);
    }
}

void InsertCharAt(StringBuffer *buffer, i32 at, i32 ch)
{
    DoubleCapacityIfFull(buffer);

    buffer->size += 1;
    MoveBytesRight(buffer->content + at, buffer->size - at);
    *(buffer->content + at) = ch;
    PlaceLineEnd(buffer);
}

inline void InsertCharAtEnd(StringBuffer *buffer, i32 ch)
{
    InsertCharAt(buffer, buffer->size, ch);
}


void RemoveCharAt(StringBuffer *buffer, i32 at)
{
    MoveBytesLeft(buffer->content + at, buffer->size - at);
    buffer->size--;
    PlaceLineEnd(buffer);
}


StringBuffer ReadFileIntoDoubledSizedBuffer(char *path)
{
    u32 fileSize = GetMyFileSize(path);
    StringBuffer res = {
        .capacity = fileSize * 2,
        .size = fileSize,
        .content = 0
    }; 
    res.content = VirtualAllocateMemory(res.capacity);
    ReadFileInto(path, fileSize, res.content);

    //removing windows new lines delimeters, assuming no two CR are next to each other
    for(int i = 0; i < fileSize; i++){
        if(*(res.content + i) == '\r')
            RemoveCharAt(&res, i);
    }

    PlaceLineEnd(&res);
    return res;
}

StringBuffer StringBufferInit(i32 size, char* content)
{
    StringBuffer res = {
        .capacity = size * 2,
        .size = size,
        .content = 0
    }; 
    res.content = VirtualAllocateMemory(res.capacity);
    char* ch = res.content;
    for(i32 i = 0; i < size; i++)
    {
        *ch = *(content + i);
        ch++;
    }
    PlaceLineEnd(&res);
    return res;
}

StringBuffer StringBufferEmptyWithCapacity(i32 capacity)
{
    StringBuffer res = {
        .capacity = capacity,
        .size = 0,
        .content = 0
    }; 
    res.content = VirtualAllocateMemory(res.capacity);
    PlaceLineEnd(&res);
    return res;
}

void StringBuffer_AppendStr(StringBuffer* buffer, char* str)
{
    //TODO: this is slow, better to calc length of str and move bytes once, not per char in str
    char* ch = str;
    while(*ch)
    {
        InsertCharAt(buffer, buffer->size, *ch);
        ch++;
    }
}
void StringBuffer_AppendStrBuff(StringBuffer* buffer, StringBuffer* bufferToAppend)
{
    //TODO: this is slow, better to calc length of str and move bytes once, not per char in str
    char* ch = bufferToAppend->content;
    while(*ch)
    {
        InsertCharAt(buffer, buffer->size, *ch);
        ch++;
    }
}

i32 IndexAfter(StringBuffer* buffer, i32 after, char ch)
{
    for(int i = after + 1; i < buffer->size; i++)
    {
        if(*(buffer->content + i) == ch)
            return i;
    }
    return -1;
}


i32 IndexBefore(StringBuffer* buffer, i32 before, char ch)
{
    for(int i = before - 1; i >= 0; i--)
    {
        if(*(buffer->content + i) == ch)
            return i;
    }
    return -1;
}