#pragma once

// Ugly function taken from the ChatFuckYouPT
// Will need to rewrite this with custom string type which has a length
void ReverseString(char *str) {
    if (!str)
        return;

    int length = 0;
    while (str[length] != '\0') {
        length++;
    }

    int start = 0;
    
    int end = length - 1;
    char temp;

    while (start < end) {
        // Swap characters at start and end indices
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;

        // Move towards the center
        start++;
        end--;
    }
}

int FormatNumber(int val, char *buff)
{
    char* start = buff;
    int isNegative = val < 0;
    if(isNegative)
        val = -val;


    if(val == 0)
    {
        *buff = '0';
        buff++;
    }

    while(val != 0) 
    {
        *buff = '0' + val % 10;
        val /= 10;
        buff++;
    }

    if(isNegative)
    {
        *buff = '-';
        buff++;
    }
    *buff = '\0';
    ReverseString(start);
    return buff - start;
}
