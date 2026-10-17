#ifndef DSM_NETWORK_H
#define DSM_NETWORK_H

#include <stddef.h>
#include "dsm/types.h"

#define DSM_LINE_CAPACITY 128

typedef struct {
    char data[DSM_LINE_CAPACITY];
    size_t used;
} DSMLineBuffer;

/* Descriptor functions return -1 on error and preserve errno. */
int dsm_network_create_server(int port);
int dsm_network_accept_peer(int server_fd);
int dsm_network_connect_peer(int port);
void dsm_network_close(int *fd);

/* Send every byte; suppress SIGPIPE so a closed peer is an ordinary error. */
DSMStatus dsm_network_send(int fd, const char *message);
/* Nonblocking, incremental newline framing; data excludes the newline on OK.
 * AGAIN keeps partial data, CLOSED means clean EOF, ERROR sets errno.
 * Reset used to zero after consuming a complete message. */
DSMStatus dsm_network_receive(int fd, DSMLineBuffer *line);

#define DSM_SEND_CAPACITY 65536u
typedef struct {
    unsigned char data[DSM_HEADER_BYTES + DSM_PAGE_BYTES];
    size_t used;
    size_t target;
} DSMReceiveBuffer;
typedef struct {
    unsigned char data[DSM_SEND_CAPACITY];
    size_t used;
    size_t sent;
} DSMSendBuffer;

DSMStatus dsm_network_queue(DSMSendBuffer *buffer, const DSMMessage *message);
DSMStatus dsm_network_flush(int fd, DSMSendBuffer *buffer);
DSMStatus dsm_network_receive_message(int fd, DSMReceiveBuffer *buffer, DSMMessage *message);

#endif /* DSM_NETWORK_H */
