#pragma once

#include "types.h"
#include "arena.c"
#include "string.c"

typedef struct Item
{
    StringBuffer title;
    struct Item* firstChild;
    struct Item* nextSibling;
    struct Item* parent;
    bool isClosed;
} Item;

typedef struct ItemEntry
{
    i32 start, end, level;
    Item *item;
    u32 flags;
} ItemEntry;


void ParseFileContent(Item *root, FileContent file, Arena* arena)
{
    ItemEntry slices[512];
    i32 slicesCount = 0;

    i32 currentLevel = 0;
    bool isReadingLevel = 1;

    i32 lineStart = 0;
    for(i32 i = 0; i < file.size; i++)
    {
        char ch = *(file.content + i);
        if(ch == ' ' && isReadingLevel)
        {
            currentLevel++;
        }
        else if (ch != ' ' && isReadingLevel)
        {
            isReadingLevel = 0;
        }

        if(*(file.content + i) == '\n')
        {
            ItemEntry entry = {.start = lineStart + currentLevel, .end = i - 1, .level = currentLevel };
            slices[slicesCount++] = entry;
            lineStart = i + 1;
            currentLevel = 0;
            isReadingLevel = 1;
        }
        else if (i == file.size - 1)
        {
            ItemEntry entry = {.start = lineStart, .end = i, .level = currentLevel };
            slices[slicesCount++] = entry;
        }
    }

    Item *stack[512] = {root};
    i32 stackLevels[512] = {-1};
    i32 currentInStack = 0;

    for(i32 i = 0; i < slicesCount; i++)
    {
        Item* current = (Item*) ArenaPush(arena, sizeof(Item));
        ItemEntry entry = slices[i];
        
        current->title = StringBufferInit(entry.end - entry.start + 2, file.content + entry.start);

        if(stackLevels[currentInStack] < entry.level)
        {
            stack[currentInStack]->firstChild = current;
            current->parent = stack[currentInStack];
            stack[++currentInStack] = current;
            stackLevels[currentInStack] = entry.level;
        }
        else if (stackLevels[currentInStack] == entry.level)
        {
            stack[currentInStack]->nextSibling = current;
            current->parent = stack[currentInStack]->parent;
            stack[currentInStack] = current;
        }
        else
        {
            Item* next = stack[--currentInStack]; 
            while(entry.level < stackLevels[currentInStack])
                next = stack[--currentInStack]; 
            
            next->nextSibling = current;
            current->parent = next->parent;

            stack[currentInStack] = current;
            stackLevels[currentInStack] = entry.level;
        }
    }
}
