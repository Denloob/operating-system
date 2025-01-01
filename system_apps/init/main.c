#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>

int main()
{
    execve_new("sh.exe", NULL);

    while (true) { /* hang */ }; // TODO: exit(0)
}
