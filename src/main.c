#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsm/node.h"

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: dsm-node [--help]");
        puts("Initial scaffold only. Networking and DSM commands are not implemented.");
        return EXIT_SUCCESS;
    }

    if (argc != 1) {
        fputs("Unsupported arguments. Use --help.\n", stderr);
        return EXIT_FAILURE;
    }

    DSMStatus status = dsm_node_run();
    if (status == DSM_NOT_IMPLEMENTED) {
        puts("DSM scaffold: modules are stubs; no simulation is running.");
        return EXIT_SUCCESS;
    }

    return status == DSM_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
