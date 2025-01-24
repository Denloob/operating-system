#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>

int main()
{
#define PATH "/dev/tty"
#define TEXT "Hello, World!\n"

    syscall(SYS_write, PATH, TEXT, sizeof(TEXT) - 1);

    return 0;
}
