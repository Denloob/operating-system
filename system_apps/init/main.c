#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>

int main()
{
    execve_new("/bin/mdsktop", NULL);

    return 0;
}
