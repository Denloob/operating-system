#include <stdint.h>
#include "FAT16.h"
#include "io.h"

void shutdown();

void reboot();

void echo_command(char *command);

void clear_command();

void ls_command(fat16_Ref *fat16 , const char* flag);

void touch_command(fat16_Ref *fat16 , const char* second_part_of_command);

int compare_strings(char *option , char* command);

void parse_command(char* command , int max_size , fat16_Ref *fat16);
