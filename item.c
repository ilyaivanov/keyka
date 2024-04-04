#pragma once

#include "types.h"
#include "arena.c"

typedef struct Item
{
    char* title;
    struct Item* firstChild;
    struct Item* nextSibling;
} Item;

void ItemSetChildren(Item* item, char* names[], i32 namesCount, Arena* arena)
{
    item->firstChild = (Item*)ArenaPush(arena, sizeof(Item));
    Item* current = item->firstChild;
    current->title = names[0];
    for(int i = 1; i < namesCount; i++)
    {
        current->nextSibling = (Item*)ArenaPush(arena, sizeof(Item));
        current = current->nextSibling;
        current->title = names[i];
    }
}


// TODO: this needs to go down into all children
i32 ItemGetTotalChildrenCount(Item* item)
{
    Item* stack[256] = {item->firstChild};
    i32 currentItem = 0;

    i32 childrenCount = 0;
    Item* current = stack[currentItem];

    while (current)
    {
        childrenCount++;

        if(current->firstChild)
        {
            stack[++currentItem] = current;
            current = current->firstChild;
        }
        else if (current->nextSibling)
        {
            current = current->nextSibling;
            stack[currentItem] = current;
        }
        else 
        {
            Item* itemInStack = stack[currentItem]->nextSibling;
            while(!itemInStack && currentItem >= 0)
            {
                currentItem--;
                itemInStack = stack[currentItem]->nextSibling;
            }
            current = itemInStack;
        }
    }
    return childrenCount;
}

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
        
        char* title = (char*) ArenaPush(arena, entry.end - entry.start + 2);
        char* ch = title;
        for(i32 i = entry.start; i <= entry.end; i++)
        {
            *ch = *(file.content + i);
            ch++;
        }
        *ch = '\0';
        
        current->title = title;

        if(stackLevels[currentInStack] < entry.level)
        {
            stack[currentInStack]->firstChild = current;
            stack[++currentInStack] = current;
            stackLevels[currentInStack] = entry.level;
        }
        else if (stackLevels[currentInStack] == entry.level)
        {
            stack[currentInStack]->nextSibling = current;
            stack[currentInStack] = current;
        }
        else
        {
            Item* next = stack[--currentInStack]; 
            while(entry.level < stackLevels[currentInStack])
                next = stack[--currentInStack]; 
            
            next->nextSibling = current;
            stack[currentInStack] = current;
            stackLevels[currentInStack] = entry.level;
        }
    }
}

// void ParseFileContent2(Item *root, FileContent file)
// {
//     //
//     // Parse file
//     //

//     ItemEntry slices[512];
//     i32 slicesCount = 0;

//     i32 currentLineStart = 0;
//     int i = 0;

//     // State machines to the rescue? 
//     i32 isLineEmpty = 1;
//     i32 lineLevel = 0;
//     i32 isReadingFlags = 0;
//     i32 hasEndedReadingFlags = 0;
//     i32 flagsEndAt = 0;
//     u32 itemFlags = 0;

//     u32 needToCloseItem = 0;

//     for (; i < file.size; i++)
//     {
//         char ch = *(file.content + i);
//         if (ch == '\r')
//             continue;

//         if (ch == '[' && !hasEndedReadingFlags)
//         {
//             isReadingFlags = 1;
//             isLineEmpty = 0;
//             SetBit(&itemFlags, ItemStateFlag_IsSerializedWithPlaceholder, 1);
//         }
//         else if (ch == ']' && isReadingFlags)
//         {
//             isReadingFlags = 0;
//             hasEndedReadingFlags = 1;
//             flagsEndAt = i + 1;

//             if(!needToCloseItem)
//                 SetBit(&itemFlags, ItemStateFlag_IsOpen, 1);
//         }
//         else if (isReadingFlags)
//         {
//             if(ch == 'y')
//                 SetBit(&itemFlags, ItemStateFlag_IsDone, 1);

//             if(ch == 'h')
//                 needToCloseItem = 1;
//         }
//         else if (ch == '\n' || i == file.size - 1)
//         {
//             if (!isLineEmpty)
//             {
//                 if(flagsEndAt == 0)
//                     SetBit(&itemFlags, ItemStateFlag_IsOpen, 1);

//                 u32 start = flagsEndAt == 0 ? currentLineStart : flagsEndAt;
//                 u32 end = i == file.size - 1 ? i + 1 : i;
//                 ItemEntry entry = {.start = start, .end = end, .level = lineLevel, .flags = itemFlags};
//                 TrimEntry(&entry, file.content);

//                 slices[slicesCount++] = entry;
//             }

//             currentLineStart = i + 1;
//             isLineEmpty = 1;
//             lineLevel = 0;
//             isReadingFlags = 0;
//             hasEndedReadingFlags = 0;
//             flagsEndAt = 0;
//             itemFlags = 0;
//             needToCloseItem = 0;
//         }
//         else if (ch != ' ')
//         {
//             isLineEmpty = 0;
//         }
//         else if (isLineEmpty)
//         {
//             lineLevel++;
//         }
//     }

//     //
//     // Convert files slices into nested tree of items
//     //

//     ItemEntry stack[256] = {0};
//     i32 stackLength = 0;

//     slices[0].item = root;
//     stack[stackLength++] = (ItemEntry){0, 0, -1, root};

//     for (int i = 0; i < slicesCount; i++)
//     {
//         ItemEntry slice = slices[i];

//         while (stack[stackLength - 1].level >= slice.level)
//             stackLength--;

//         ItemEntry parentSlice = stack[stackLength - 1];

//         if (ChildCount(parentSlice.item) == 0)
//             InitChildren(parentSlice.item, 4);

//         slice.item = AllocateZeroedMemory(1, sizeof(Item));
//         slice.item->flags = slice.flags;
        
//         AppendChild(parentSlice.item, slice.item);
//         stack[stackLength++] = slice;

//         InitBuffer(&slice.item->textBuffer, file.content + slice.start, slice.end - slice.start);
//     }

//     ForEachChild(root, CloseIfEmpty);
// }
