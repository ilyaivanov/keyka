#pragma once

#include "types.h"
#include "arena.c"

typedef enum ChangeType
{
    Rename, Remove, Move, Create
} ChangeType;

typedef struct Change
{
    ChangeType type;
    StringBuffer oldTitle;
    StringBuffer newTitle;
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
    change->oldTitle.content = VirtualAllocateMemory(item->title.capacity);
    change->oldTitle.capacity = item->title.capacity;
    change->oldTitle.size = item->title.size;
    change->newTitle = item->title;
    memcpy(change->oldTitle.content, item->title.content, item->title.capacity);
}

void UndoLastChange(ChangeHistory* history)
{
    if(history->currentChange > 0)
    {
        Change* change = &history->changes[--history->currentChange];

        //TODO: memory leak - newTitle will be lost on next BeforeRenameItem call
        change->item->title = change->oldTitle;
    }
}

void RedoLastChange(ChangeHistory* history)
{
    if(history->currentChange < history->totalChanges)
    {
        Change* change = &history->changes[history->currentChange++];
        
        //TODO: memory leak - oldTitle will be lost on next BeforeRenameItem call
        change->item->title = change->newTitle;
    }
}







