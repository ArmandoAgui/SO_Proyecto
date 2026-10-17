#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsm/node.h"

static void print_help(const char *program)
{
    printf("Uso: %s --id <0|1> --port <1..65535> --peer-port <1..65535>\n", program);
    puts("Dos procesos en 127.0.0.1: nodo 0 conecta; nodo 1 acepta.");
    puts("Ejemplo nodo 0: --id 0 --port 5000 --peer-port 5001");
    puts("Ejemplo nodo 1: --id 1 --port 5001 --peer-port 5000");
    puts("Cierre: Enter, exit, EOF o Ctrl+C. Ayuda: --help.");
}

static int parse_number(const char *text, int maximum, int *value)
{
    if (*text == '\0') {
        return 0;
    }
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
    }
    errno = 0;
    char *end;
    long number = strtol(text, &end, 10);
    if (errno != 0 || *end != '\0' || number > maximum) {
        return 0;
    }
    *value = (int)number;
    return 1;
}

int main(int argc, char **argv)
{
    DSMNode node = {.id = -1, .listen_port = -1, .peer_port = -1,
                    .server_fd = -1, .peer_fd = -1};
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return EXIT_SUCCESS;
    }
    for (int i = 1; i < argc; ++i) {
        int *destination;
        int maximum = 65535;
        if (strcmp(argv[i], "--id") == 0) {
            destination = &node.id;
            maximum = 1;
        } else if (strcmp(argv[i], "--port") == 0) {
            destination = &node.listen_port;
        } else if (strcmp(argv[i], "--peer-port") == 0) {
            destination = &node.peer_port;
        } else {
            fprintf(stderr, "Error: argumento desconocido '%s'. Use --help.\n", argv[i]);
            return EXIT_FAILURE;
        }
        if (*destination != -1) {
            fprintf(stderr, "Error: argumento repetido '%s'.\n", argv[i]);
            return EXIT_FAILURE;
        }
        if (i + 1 == argc) {
            fprintf(stderr, "Error: falta el valor de '%s'. Use --help.\n", argv[i]);
            return EXIT_FAILURE;
        }
        if (!parse_number(argv[i + 1], maximum, destination) ||
            (maximum == 65535 && *destination == 0)) {
            fprintf(stderr, "Error: valor inválido para '%s': '%s'. Use --help.\n",
                    argv[i], argv[i + 1]);
            return EXIT_FAILURE;
        }
        ++i;
    }
    if (node.id < 0 || node.listen_port < 0 || node.peer_port < 0) {
        fputs("Error: se requieren --id, --port y --peer-port. Use --help.\n", stderr);
        return EXIT_FAILURE;
    }
    if (node.listen_port == node.peer_port) {
        fputs("Error: el puerto local y el del peer deben ser distintos.\n", stderr);
        return EXIT_FAILURE;
    }
    return dsm_node_run(&node) == DSM_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
