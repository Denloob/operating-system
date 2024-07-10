#include "io.h"

void start(short param)
{
    while (1) {
        wait_key();
        puts("You pressed a key!");
    }
}
