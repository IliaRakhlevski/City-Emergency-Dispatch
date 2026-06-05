#include <time.h>
#include <stdio.h>
#include <string.h>
#include "init.h"
#include "event_generator.h"
#include "server.h"
#include "client.h"


#define MODE_SERVER "-server"
#define MODE_CLIENT "-client"


/**
 * @brief Application entry point.
 *
 * Initializes the runtime environment, processes
 * command-line arguments, and starts either the
 * server or client application mode.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument array.
 *
 * @return 0 on success, non-zero on error.
 */
int main(int argc, char* argv[]) {

    // Initialize application infrastructure.
    init_main();

    // Seed random number generator.
    srand((unsigned int)time(NULL));

    if (argc < 2)
    {
        printf("Usage: %s -server | -client\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], MODE_SERVER) == 0)
    {
        RunServer();
    }
    else if (strcmp(argv[1], MODE_CLIENT) == 0)
    {
        RunClient();
    }
    else
    {
        printf("Unknown mode: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
