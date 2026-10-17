#define _POSIX_C_SOURCE 200809L
#include "dsm/node.h"
#include "dsm/network.h"
#include "dsm/protocol.h"
#include <stdlib.h>

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
            int result = poll(&listener, 1, 100);
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return node_error(node, "esperar conexión");
            }
            if (result == 0) continue;
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

typedef struct {
    DSMNode *node;
    DSMProtocol protocol;
    DSMSendBuffer outgoing;
    DSMReceiveBuffer incoming;
    int initialized;
} DSMSession;

static DSMStatus queue_message(void *context, const DSMMessage *message)
{
    DSMSession *session = context;
    printf("[NODE %d] SEND %s origin=%u req=%u page=%u version=%u flags=%u bytes=%u\n",
           session->node->id, dsm_protocol_message_name(message->type), message->origin_id,
           message->request_id, message->page_id, message->page_version,
           message->flags, message->payload_len);
    return dsm_network_queue(&session->outgoing, message);
}

static DSMStatus session_loop(DSMSession *session)
{
    DSMNode *node = session->node;
    DSMLineBuffer hello = {0};
    int hello_complete = 0;
    int exiting = 0;
    int command_overflow = 0;
    size_t command_used = 0;
    char command_line[DSM_PAGE_BYTES + 64];
    int64_t hello_started = dsm_monotonic_ms();
    int64_t frame_started = 0, send_started = 0;
    if (node->id == 0 && send_hello(node) != DSM_OK) return DSM_ERROR;
    while (!stop_requested) {
        if (exiting && !dsm_protocol_busy(&session->protocol) && session->outgoing.used == 0) {
            return DSM_OK;
        }
        short socket_events = POLLIN;
        if (session->outgoing.used) socket_events |= POLLOUT;
        struct pollfd descriptors[2] = {
            {.fd = node->peer_fd, .events = socket_events},
            {.fd = session->protocol.peer_ready && !exiting ? STDIN_FILENO : -1, .events = POLLIN}
        };
        int result = poll(descriptors, 2, 100);
        if (result < 0) {
            if (errno == EINTR) continue;
            return node_error(node, "esperar eventos");
        }
        int64_t now = dsm_monotonic_ms();
        if ((!session->protocol.peer_ready && now - hello_started >= HELLO_TIMEOUT_MS) ||
            dsm_protocol_expired(&session->protocol, now) ||
            (frame_started && now - frame_started >= DSM_OPERATION_TIMEOUT_MS) ||
            (send_started && now - send_started >= DSM_OPERATION_TIMEOUT_MS)) {
            errno = ETIMEDOUT;
            return node_error(node, !session->protocol.peer_ready ? "esperar HELLO" : "esperar operación/trama DSM");
        }
        if (descriptors[0].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
            if (!hello_complete) {
                DSMStatus status = dsm_network_receive(node->peer_fd, &hello);
                if (status == DSM_ERROR) return node_error(node, "recibir mensaje");
                if (status == DSM_CLOSED) {
                    errno = EPROTO;
                    return node_error(node, "completar HELLO (cierre prematuro)");
                }
                if (status == DSM_OK) {
                    const char *expected = node->id == 0 ? "HELLO_ACK 1" : "HELLO 0";
                    printf("[NODE %d] Mensaje recibido: %s\n", node->id, hello.data);
                    if (strcmp(hello.data, expected) != 0) {
                        errno = EPROTO;
                        return node_error(node, "validar mensaje HELLO");
                    }
                    if (node->id == 1 && send_hello(node) != DSM_OK) return DSM_ERROR;
                    hello_complete = 1;
                    printf("[NODE %d] Intercambio HELLO completo\n", node->id);
                    if (dsm_protocol_init(&session->protocol, node->id, queue_message, session) != DSM_OK) {
                        return node_error(node, "reservar memoria local");
                    }
                    session->initialized = 1;
                    if (dsm_protocol_start(&session->protocol) != DSM_OK) return DSM_ERROR;
                }
            } else {
                /* Bound each dispatch batch so stdin and deadlines stay responsive. */
                for (int i = 0; i < 32; ++i) {
                    DSMMessage message = {0};
                    DSMStatus status = dsm_network_receive_message(node->peer_fd, &session->incoming, &message);
                    if (status == DSM_AGAIN) {
                        if (session->incoming.used && !frame_started) frame_started = now;
                        break;
                    }
                    if (status == DSM_ERROR) return node_error(node, "recibir mensaje DSM");
                    if (status == DSM_CLOSED) {
                        printf("[NODE %d] Peer desconectado; conexión cerrada por el peer\n", node->id);
                        if (!session->protocol.peer_ready || dsm_protocol_busy(&session->protocol)) {
                            errno = EPROTO;
                            return node_error(node, "completar sesión DSM (cierre con operación pendiente)");
                        }
                        return DSM_OK;
                    }
                    frame_started = 0;
                    printf("[NODE %d] RECV %s origin=%u req=%u page=%u version=%u flags=%u bytes=%u\n",
                           node->id, dsm_protocol_message_name(message.type), message.origin_id,
                           message.request_id, message.page_id, message.page_version,
                           message.flags, message.payload_len);
                    if (dsm_protocol_on_message(&session->protocol, &message) != DSM_OK) return DSM_ERROR;
                }
            }
        }
        if (descriptors[1].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
            char byte;
            ssize_t count = read(STDIN_FILENO, &byte, 1);
            if (count < 0) {
                if (errno == EINTR) continue;
                return node_error(node, "leer entrada");
            }
            if (count == 0) {
                puts("Fin de entrada; esperando operación pendiente antes del cierre");
                exiting = 1;
            } else if (byte == '\n') {
                command_line[command_used] = '\0';
                DSMCommand command;
                if (command_overflow || dsm_cli_parse(command_line, &command) != DSM_OK) {
                    printf("[NODE %d] Error: comando inválido; use help (páginas 0..255, texto 1..4095 bytes)\n", node->id);
                } else if (command.type == CLI_EMPTY || command.type == CLI_EXIT) {
                    printf("[NODE %d] Cierre solicitado por el usuario; esperando operación pendiente\n", node->id);
                    exiting = 1;
                } else if (command.type == CLI_HELP) {
                    dsm_cli_print_help();
                } else if (command.type == CLI_STATUS) {
                    dsm_protocol_status(&session->protocol, command.has_page, command.page);
                } else if (dsm_protocol_command(&session->protocol, &command) != DSM_OK) {
                    return DSM_ERROR;
                }
                command_used = 0;
                command_overflow = 0;
            } else if (byte == '\0') {
                command_overflow = 1;
            } else if (command_used < sizeof(command_line) - 1) {
                command_line[command_used++] = byte;
            } else {
                command_overflow = 1;
            }
        }
        if (session->outgoing.used) {
            if (!send_started) send_started = now;
            DSMStatus status = dsm_network_flush(node->peer_fd, &session->outgoing);
            if (status == DSM_ERROR) return node_error(node, "enviar mensaje DSM");
            if (status == DSM_OK) send_started = 0;
        }
    }
    return DSM_OK;
}

static DSMStatus run_session(DSMNode *node)
{
    DSMSession *session = calloc(1, sizeof(*session));
    if (session == NULL) return node_error(node, "reservar sesión");
    session->node = node;
    DSMStatus status = session_loop(session);
    if (session->initialized) {
        dsm_protocol_status(&session->protocol, 0, 0);
        dsm_protocol_destroy(&session->protocol);
    }
    free(session);
    return status;
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
