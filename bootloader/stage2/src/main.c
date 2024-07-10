#include "io.h"

void start(short param)
{
    while (1) {
        int key = wait_key();
        #define MSG "You pressed a key! That key was: "
        char msg[sizeof(MSG)+1] = MSG;
        msg[sizeof(MSG)-1] = key_to_char(key);
        msg[sizeof(MSG)] = '\0';
        puts(msg);
    }
}
