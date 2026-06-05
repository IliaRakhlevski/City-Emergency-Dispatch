#include "FreeRTOS.h"
#include "main_config.h"
#include "semphr.h"
#include "task.h"
#include <stdio.h>
#include <unistd.h>
/*
 * Signal handler for Ctrl_C to cause the program to exit, and generate the
 * profiling info.
 */
static void handle_sigint(int signal);

#define BUILD BUILD_DIR

void init_main(void);