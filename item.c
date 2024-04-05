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

i32 ItemGetTotalChildrenCount(Item* parent)
{
    //Traversal
    if(!parent->firstChild)
        return 0;

    Item* stack[512] = {parent->firstChild};
    i32 currentItem = 0;
    i32 res = 0;
    Item* item = stack[currentItem];

    while (item)
    {
        res++;
        if (item->firstChild && !item->isClosed)
        {
            stack[++currentItem] = item->firstChild;
            item = item->firstChild;
        }
        else if (item->nextSibling)
        {
            item = item->nextSibling;
            stack[currentItem] = item;
        }
        else
        {
            if (currentItem == 0)
                item = 0;
            else
            {
                Item *itemInStack = stack[--currentItem];
                while (currentItem >= 0 && !itemInStack->nextSibling)
                    itemInStack = stack[--currentItem];

                if (itemInStack && itemInStack->nextSibling)
                    item = itemInStack->nextSibling;
                else
                    item = 0;
            }
        }
    }

    return res;
}


void ParseFileContent(Item *root, StringBuffer file, Arena* arena)
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
        
        current->title = StringBufferInit(entry.end - entry.start + 1, file.content + entry.start);

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



void SaveStateIntoFile(Item* root, char* path)
{
    StringBuffer s = StringBufferEmptyWithCapacity(256);

    //Traversal
    Item* stack[512] = {root->firstChild};
    i32 stackLevels[512] = {0};
    i32 currentItem = 0;

    Item* item = stack[currentItem];
    while(item)
    {
        for(int i = 0; i < stackLevels[currentItem]; i++)
        {
            InsertCharAtEnd(&s, ' ');
            InsertCharAtEnd(&s, ' ');
        }
        StringBuffer_AppendStrBuff(&s, &item->title);
        InsertCharAtEnd(&s, '\r');
        InsertCharAtEnd(&s, '\n');

        if(item->firstChild)
        {
            stack[++currentItem] = item->firstChild;
            stackLevels[currentItem] = stackLevels[currentItem - 1] + 1;
            item = item->firstChild;
        }
        else if(item->nextSibling)
        {
            item = item->nextSibling;
            stack[currentItem] = item;
        }
        else
        {
            Item* itemInStack = stack[--currentItem];
            while(currentItem >= 0 && !itemInStack->nextSibling)
                itemInStack = stack[--currentItem];

                
            if(itemInStack && itemInStack->nextSibling)
            {
                item = itemInStack->nextSibling;
                stack[currentItem] = item;
            }
            else 
                item = 0;

        }
    }

    WriteMyFile(path, s.content, s.size);
}