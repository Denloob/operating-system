#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/reboot.h>

int main(int argc, char **argv)
{
    reboot(RB_AUTOBOOT);

    puts("reboot: failed");
    return 1;
}
