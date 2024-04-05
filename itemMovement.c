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

void RemoveItemFromTree(Item* item)
{

}

void InsertItemAfter(Item* afterItem, Item* itemToInsert)
{

}

void InsertItemBefore(Item* beforeItem, Item* itemToInsert)
{
    
}

void InsertItemAsFirstChild(Item* parent, Item* itemToInsert)
{
    
}

void InsertItemAsLastDirectChild(Item* parent, Item* itemToInsert)
{
    
}


void MoveItemDown(Item* item)
{
    if(item->nextSibling)
    {
        Item* prev = GetItemPrevSibling(item);
        Item* tmp = item->nextSibling;
        item->nextSibling = tmp->nextSibling;
        tmp->nextSibling = item;

        if(prev)
            prev->nextSibling = tmp;
        else 
            item->parent->firstChild = tmp;
    }
}

void MoveItemUp(Item* item)
{
    if(item->parent->firstChild != item)
    {
        Item* prev = GetItemPrevSibling(item);
        if(prev)
        {
            Item* prevPrev = GetItemPrevSibling(prev);
            if(prevPrev)
            {
                prevPrev->nextSibling = item;
                prev->nextSibling = item->nextSibling;
                item->nextSibling = prev;
            }
            else 
            {
                Item* first = item->parent->firstChild;
                item->parent->firstChild = item;
                first->nextSibling = item->nextSibling;
                item->nextSibling = first;
            }
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
            if(prev->firstChild)
            {
                Item* lastSibling = prev->firstChild;
                while(lastSibling->nextSibling)
                    lastSibling = lastSibling->nextSibling;

                lastSibling->nextSibling = item;
                item->parent = lastSibling->parent;
            }
            else 
            {
                prev->firstChild = item;
                item->parent = prev;
            }

            prev->nextSibling = item->nextSibling;
            item->nextSibling = 0;
        }
    }
}

void MoveItemLeft(Item* item)
{
    if(item->parent->parent)
    {
        Item* prev = GetItemPrevSibling(item);
        if(prev)
            prev->nextSibling = item->nextSibling;
        else if(item->nextSibling)
            item->parent->firstChild = item->nextSibling;
        else 
            item->parent->firstChild = 0;

        Item* next = item->parent->nextSibling;
        item->parent->nextSibling = item;
        item->nextSibling = next;
        item->parent = item->parent->parent;
    }
}
