#pragma once
#include "types.h"
#include "item.c"


Item* GetItemPrevSibling(Item* item)
{
    if(item->parent->firstChild == item)
        return 0;
        
    Item* prevSibling = item->parent->firstChild;

    while(prevSibling->nextSibling != item)
        prevSibling = prevSibling->nextSibling;

    return prevSibling;
}

inline bool IsRoot(Item* item)
{
    return !item->parent;
}

void RemoveItemFromTree(Item* item)
{
    Item* prev = GetItemPrevSibling(item);
    
    if(prev)
        prev->nextSibling = item->nextSibling;
    else 
        item->parent->firstChild = item->nextSibling;
}

void InsertItemAfter(Item* afterItem, Item* itemToInsert)
{
    Item* next = afterItem->nextSibling;
    afterItem->nextSibling = itemToInsert;
    itemToInsert->nextSibling = next;
    itemToInsert->parent = afterItem->parent;
}

void InsertItemAsFirstChild(Item* parent, Item* itemToInsert)
{
    Item* first = parent->firstChild;
    parent->firstChild = itemToInsert;
    itemToInsert->nextSibling = first;
    itemToInsert->parent = parent;
}

void InsertItemAsLastDirectChild(Item* parent, Item* itemToInsert)
{
    Item* lastSibling = parent->firstChild;
    while(lastSibling->nextSibling)
        lastSibling = lastSibling->nextSibling;

    InsertItemAfter(lastSibling, itemToInsert);
}


void MoveItemDown(Item* item)
{
    if(!item->nextSibling)
        return;

    RemoveItemFromTree(item);
    InsertItemAfter(item->nextSibling, item);
}

void MoveItemUp(Item* item)
{
    if(item->parent->firstChild != item)
    {
        Item* prev = GetItemPrevSibling(item);
        if(prev)
        {
            Item* prevPrev = GetItemPrevSibling(prev);
            RemoveItemFromTree(item);
            if(prevPrev)
                InsertItemAfter(prevPrev, item);
            else 
                InsertItemAsFirstChild(item->parent, item);
        }
    }
}

void MoveItemRight(Item* item)
{
    if(item->parent->firstChild != item)
    {
        Item* prev = GetItemPrevSibling(item);
        if(prev)
        {
            RemoveItemFromTree(item);
            if(prev->firstChild)
                InsertItemAsLastDirectChild(prev, item);
            else 
                InsertItemAsFirstChild(prev, item);
        }
    }
}

void MoveItemLeft(Item* item)
{
    if(!IsRoot(item->parent))
    {
        RemoveItemFromTree(item);
        InsertItemAfter(item->parent, item);
    }
}
