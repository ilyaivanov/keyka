#pragma once

#include "types.h"
#include "arena.c"
#include "itemMovement.c"

typedef enum ChangeType
{
    Rename, RemoveCharAtChange, Remove, Move, Create
} ChangeType;

typedef struct Change
{
    ChangeType type;
    union
    {
        struct Rename 
        {
            StringBuffer oldTitle;
            StringBuffer newTitle;
        } rename;
        struct RemoveCharAt2
        {
            char ch;
            u32 at;
        } removeCharAt2;
        struct RemovedItem 
        {
            Item* parent;
            u32 position;
        } removedItem;
    };
    Item* item;
} Change;


typedef struct ChangeHistory
{
    Change changes[1024 * 4];

    //points to the last UNINITIALIZED change
    u32 currentChange;
    u32 totalChanges;
} ChangeHistory;



void BeforeRenameItem(ChangeHistory* history, Item* item)
{
    // no out of bounds checks
    // no checks for changes made (memory leaks)
    Change* change = &history->changes[history->currentChange++];
    history->totalChanges = history->currentChange;
    change->item = item;
    change->type = Rename;

    change->rename.oldTitle.content = VirtualAllocateMemory(item->title.capacity);
    change->rename.oldTitle.capacity = item->title.capacity;
    change->rename.oldTitle.size = item->title.size;
    change->rename.newTitle = item->title;
    memcpy(change->rename.oldTitle.content, item->title.content, item->title.capacity);
}

void OnItemRemoved(ChangeHistory* history, Item* item)
{
    Change* change = &history->changes[history->currentChange++];
    history->totalChanges = history->currentChange;
    change->item = item;

    change->type = Remove;

    change->removedItem.parent = item->parent;

    u32 position = 0;
    Item* childs = item->parent->firstChild;
    while(childs != item)
    {
        childs = childs->nextSibling;
        position++;
    }

    change->removedItem.position = position;

}

void UndoLastChange(ChangeHistory* history)
{
    if(history->currentChange > 0)
    {
        Change* change = &history->changes[--history->currentChange];

        if(change->type == Rename)
        {
            //TODO: memory leak - newTitle will be lost on next BeforeRenameItem call
            change->item->title = change->rename.oldTitle;
        } else if (change->type == Remove)
        {
            if(change->removedItem.position == 0)
            {
                InsertItemAsFirstChild(change->item->parent, change->item);
            }else 
            {
                u32 pos = change->removedItem.position;

                Item* prev = change->item->parent->firstChild;

                while(pos > 1)
                {
                    prev = prev->nextSibling;
                    pos--;
                }
                InsertItemAfter(prev, change->item);
            }
        }
    }
}

void RedoLastChange(ChangeHistory* history)
{
    if(history->currentChange < history->totalChanges)
    {
        Change* change = &history->changes[history->currentChange++];
        
        if(change->type == Rename)
        {
            //TODO: memory leak - oldTitle will be lost on next BeforeRenameItem call
            change->item->title = change->rename.newTitle;
        } else if (change->type == Remove)
        {
            //TODO: think about how to change selectedItem from here
            RemoveItemFromTree(change->item);
        }
    }
}







