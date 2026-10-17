#define _POSIX_C_SOURCE 200809L
#include "dsm/node.h"
#include "dsm/network.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CONNECT_ATTEMPTS 30
#define HELLO_TIMEOUT_MS 10000

/* Only the signal handler writes this flag; no work or I/O in the handler. */
static volatile sig_atomic_t stop_requested = 0;

static void request_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static DSMStatus node_error(const DSMNode *node, const char *operation)
{
    fprintf(stderr, "[NODE %d] Error al %s: %s\n", node->id, operation, strerror(errno));
    return DSM_ERROR;
}

static DSMStatus send_hello(DSMNode *node)
{
    const char *message = node->id == 0 ? "HELLO 0\n" : "HELLO_ACK 1\n";
    if (dsm_network_send(node->peer_fd, message) != DSM_OK) {
        return node_error(node, "enviar mensaje");
    }
    printf("[NODE %d] Mensaje enviado: %s", node->id, message);
    return DSM_OK;
}

static DSMStatus establish_connection(DSMNode *node)
{
    if (node->id == 1) {
        printf("[NODE 1] Esperando conexión de Nodo 0\n");
        struct pollfd listener = {.fd = node->server_fd, .events = POLLIN};
        while (!stop_requested) {
            int result = poll(&listener, 1, -1);
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return node_error(node, "esperar conexión");
            }
            node->peer_fd = dsm_network_accept_peer(node->server_fd);
            if (node->peer_fd < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return node_error(node, "aceptar conexión");
            }
            puts("[NODE 1] Conexión recibida y aceptada desde 127.0.0.1");
            return DSM_OK;
        }
        return DSM_OK;
    }

    for (int attempt = 1; attempt <= CONNECT_ATTEMPTS && !stop_requested; ++attempt) {
        printf("[NODE 0] Intentando conectar con 127.0.0.1:%d (%d/%d)\n",
               node->peer_port, attempt, CONNECT_ATTEMPTS);
        node->peer_fd = dsm_network_connect_peer(node->peer_port);
        if (node->peer_fd >= 0) {
            puts("[NODE 0] Conexión establecida con Nodo 1");
            return DSM_OK;
        }
        if (errno == EINTR && stop_requested) {
            return DSM_OK;
        }
        if (errno != ECONNREFUSED || attempt == CONNECT_ATTEMPTS) {
            return node_error(node, "conectar con peer");
        }
        puts("[NODE 0] Peer todavía no disponible. Reintentando en 1 segundo...");
        if (poll(NULL, 0, 1000) < 0 && errno != EINTR) {
            return node_error(node, "esperar reintento");
        }
    }
    return DSM_OK;
}

static DSMStatus run_session(DSMNode *node)
{
    DSMLineBuffer line = {0};
    int hello_complete = 0;
    char command[16];
    size_t command_used = 0;
    int command_overflow = 0;
    if (node->id == 0 && send_hello(node) != DSM_OK) {
        return DSM_ERROR;
    }
    while (!stop_requested) {
        struct pollfd descriptors[2] = {
            {.fd = node->peer_fd, .events = POLLIN},
            {.fd = hello_complete ? STDIN_FILENO : -1, .events = POLLIN}
        };
        /* Receive never blocks on a partial line. A silent/incomplete HELLO
         * times out after 10 seconds without socket activity. */
        int result = poll(descriptors, 2, hello_complete ? -1 : HELLO_TIMEOUT_MS);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return node_error(node, "esperar eventos");
        }
        if (result == 0) {
            errno = ETIMEDOUT;
            return node_error(node, "esperar HELLO");
        }
        if (descriptors[0].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
            DSMStatus status = dsm_network_receive(node->peer_fd, &line);
            if (status == DSM_ERROR) {
                return node_error(node, "recibir mensaje");
            }
            if (status == DSM_CLOSED) {
                printf("[NODE %d] Peer desconectado; conexión cerrada por el peer\n", node->id);
                if (!hello_complete) {
                    errno = EPROTO;
                    return node_error(node, "completar HELLO (cierre prematuro)");
                }
                return DSM_OK;
            }
            if (status == DSM_OK) {
                const char *expected = node->id == 0 ? "HELLO_ACK 1" : "HELLO 0";
                printf("[NODE %d] Mensaje recibido: %s\n", node->id, line.data);
                if (hello_complete || strcmp(line.data, expected) != 0) {
                    errno = EPROTO;
                    return node_error(node, "validar mensaje HELLO");
                }
                line.used = 0;
                if (node->id == 1 && send_hello(node) != DSM_OK) {
                    return DSM_ERROR;
                }
                hello_complete = 1;
                printf("[NODE %d] Intercambio HELLO completo. Enter o exit para cerrar.\n", node->id);
            }
        }
        if (descriptors[1].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
            char byte;
            ssize_t count = read(STDIN_FILENO, &byte, 1);
            if (count < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return node_error(node, "leer entrada");
            }
            if (count == 0) {
                printf("[NODE %d] Fin de entrada; cerrando\n", node->id);
                return DSM_OK;
            }
            if (byte == '\n') {
                command[command_used] = '\0';
                if (!command_overflow && (command_used == 0 || strcmp(command, "exit") == 0)) {
                    printf("[NODE %d] Cierre solicitado por el usuario\n", node->id);
                    return DSM_OK;
                }
                puts("Comando desconocido. Use Enter o exit para cerrar.");
                command_used = 0;
                command_overflow = 0;
            } else if (command_used < sizeof(command) - 1) {
                command[command_used++] = byte;
            } else {
                command_overflow = 1;
            }
        }
    }
    return DSM_OK;
}

DSMStatus dsm_node_run(DSMNode *node)
{
    struct sigaction action = {0};
    action.sa_handler = request_stop;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) < 0 || sigaction(SIGTERM, &action, NULL) < 0) {
        return node_error(node, "configurar señales");
    }
    stop_requested = 0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("[NODE %d] Iniciando nodo\n", node->id);
    node->server_fd = dsm_network_create_server(node->listen_port);
    if (node->server_fd < 0) {
        return node_error(node, "crear servidor TCP (socket/SO_REUSEADDR/bind/listen)");
    }
    printf("[NODE %d] Escuchando en 127.0.0.1:%d\n", node->id, node->listen_port);
    DSMStatus status = establish_connection(node);
    if (status == DSM_OK && node->peer_fd >= 0 && !stop_requested) {
        status = run_session(node);
    }
    if (stop_requested) {
        printf("[NODE %d] Señal de cierre recibida\n", node->id);
    }
    if (node->peer_fd >= 0) {
        dsm_network_close(&node->peer_fd);
        printf("[NODE %d] Conexión cerrada\n", node->id);
    }
    dsm_network_close(&node->server_fd);
    printf("[NODE %d] Socket servidor cerrado. Nodo finalizado\n", node->id);
    return status;
}
