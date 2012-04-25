#include "misc.h"

void Delay(u32 count)
{
    while(count) {
        int i = 0; //72000;
        while(i)
            i--;
        count--;
    }
}

void Hang()
{
    while(1)
        ;
}
