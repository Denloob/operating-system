#include <stdint.h>
#include "io.h"

void shutdown();

void reboot();

void echo_command(char *command , int max_size);

int compare_strings(char *option , char* command);

void parse_command(char* command , int max_size);
