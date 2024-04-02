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
    // Item* stack[256] = {item};
    // i32 currentItem = 0;

    i32 childrenCount = 1;
    Item* child = item->firstChild;

    while (child)
    {
        childrenCount++;
        child = child->nextSibling;
    }
    return childrenCount;
}