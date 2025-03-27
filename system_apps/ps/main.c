#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <stdint.h>

#define MAX_PROCESSES 5

typedef enum
{
    PCB_STATE_TERMINATED,
    PCB_STATE_RUNNING,
    PCB_STATE_WAITING_FOR_IO,
    PCB_STATE_READY,
    PCB_STATE_ZOMBIE,
} pcb_State;

const char *state_to_string(int state)
{
    switch (state)
    {
    case PCB_STATE_READY: return "READY     ";
    case PCB_STATE_RUNNING: return "RUNNING   ";
    case PCB_STATE_WAITING_FOR_IO: return "WAITING   ";
    case PCB_STATE_ZOMBIE: return "TERMINATED";
    default: return "UNKNOWN   ";
    }
}

int main(void)
{
    ProcessInfo processes[MAX_PROCESSES] = {0};
    int count = get_processes(processes, MAX_PROCESSES);

    if (count < 0)
    {
        printf("ps: failed to retrieve processes\n");
        return 1;
    }

    printf(" PID     STATE       NAME            CWD\n");
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < count; i++)
    {
        printf("0x%llx %s %s %s\n",
            (unsigned long long)processes[i].pid,
            state_to_string(processes[i].state),
            processes[i].name,
            processes[i].cwd);
    }

    printf("\nTotal: %d processes\n", count);
    return 0;
}
