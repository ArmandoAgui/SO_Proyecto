#include "dsm/network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static struct sockaddr_in local_address(int port)
{
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return address;
}

int dsm_network_create_server(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    int reuse = 1;
    struct sockaddr_in address = local_address(port);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0 ||
        bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(fd, 1) < 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    return fd;
}

int dsm_network_accept_peer(int server_fd)
{
    return accept(server_fd, NULL, NULL);
}

int dsm_network_connect_peer(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_in address = local_address(port);
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    return fd;
}

void dsm_network_close(int *fd)
{
    if (*fd >= 0) {
        /* On Linux, do not retry close after EINTR: the fd is already released. */
        close(*fd);
        *fd = -1;
    }
}

DSMStatus dsm_network_send(int fd, const char *message)
{
    size_t sent = 0;
    size_t length = strlen(message);
    while (sent < length) {
        ssize_t count = send(fd, message + sent, length - sent, MSG_NOSIGNAL);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return DSM_ERROR;
        }
        if (count == 0) {
            errno = EPIPE;
            return DSM_ERROR;
        }
        sent += (size_t)count;
    }
    return DSM_OK;
}

DSMStatus dsm_network_receive(int fd, DSMLineBuffer *line)
{
    /* One byte at a time is sufficient for this small teaching protocol and
     * leaves the following frame in the socket when lines arrive together. */
    for (;;) {
        char byte;
        ssize_t count = recv(fd, &byte, 1, MSG_DONTWAIT);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return errno == EAGAIN || errno == EWOULDBLOCK ? DSM_AGAIN : DSM_ERROR;
        }
        if (count == 0) {
            if (line->used != 0) {
                errno = EPROTO; /* EOF in the middle of a frame. */
                return DSM_ERROR;
            }
            return DSM_CLOSED;
        }
        if (byte == '\n') {
            line->data[line->used] = '\0';
            return DSM_OK;
        }
        if (byte == '\0' || line->used >= sizeof(line->data) - 1) {
            errno = EMSGSIZE;
            return DSM_ERROR;
        }
        line->data[line->used++] = byte;
    }
}

static void put_u32(unsigned char *destination, uint32_t value)
{
    value = htonl(value);
    memcpy(destination, &value, sizeof(value));
}

static uint32_t get_u32(const unsigned char *source)
{
    uint32_t value;
    memcpy(&value, source, sizeof(value));
    return ntohl(value);
}

DSMStatus dsm_network_queue(DSMSendBuffer *buffer, const DSMMessage *message)
{
    size_t length = DSM_HEADER_BYTES + message->payload_len;
    if (message->payload_len > DSM_PAGE_BYTES) {
        errno = EMSGSIZE;
        return DSM_ERROR;
    }
    if (buffer->sent != 0) {
        memmove(buffer->data, buffer->data + buffer->sent, buffer->used - buffer->sent);
        buffer->used -= buffer->sent;
        buffer->sent = 0;
    }
    if (length > sizeof(buffer->data) - buffer->used) {
        errno = ENOBUFS;
        return DSM_ERROR;
    }
    unsigned char *destination = buffer->data + buffer->used;
    uint32_t fields[10] = {DSM_WIRE_MAGIC, DSM_PROTOCOL_VERSION, message->type,
        message->origin_id, message->request_id, message->page_id, message->owner_id,
        message->page_version, message->flags, message->payload_len};
    for (size_t i = 0; i < 10; ++i) put_u32(destination + i * 4, fields[i]);
    memcpy(destination + DSM_HEADER_BYTES, message->payload, message->payload_len);
    buffer->used += length;
    return DSM_OK;
}

DSMStatus dsm_network_flush(int fd, DSMSendBuffer *buffer)
{
    while (buffer->sent < buffer->used) {
        ssize_t count = send(fd, buffer->data + buffer->sent,
                             buffer->used - buffer->sent, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (count < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return DSM_AGAIN;
            return DSM_ERROR;
        }
        if (count == 0) {
            errno = EPIPE;
            return DSM_ERROR;
        }
        buffer->sent += (size_t)count;
    }
    buffer->used = buffer->sent = 0;
    return DSM_OK;
}

static DSMStatus decode_header(const unsigned char *source, DSMMessage *message)
{
    if (get_u32(source) != DSM_WIRE_MAGIC || get_u32(source + 4) != DSM_PROTOCOL_VERSION) {
        errno = EPROTO;
        return DSM_ERROR;
    }
    message->type = get_u32(source + 8);
    message->origin_id = get_u32(source + 12);
    message->request_id = get_u32(source + 16);
    message->page_id = get_u32(source + 20);
    message->owner_id = get_u32(source + 24);
    message->page_version = get_u32(source + 28);
    message->flags = get_u32(source + 32);
    message->payload_len = get_u32(source + 36);
    uint32_t expected_length = message->type == MSG_HELLO ? 12u :
                               message->type == MSG_PAGE_DATA ? DSM_PAGE_BYTES : 0u;
    if (message->type < MSG_HELLO || message->type > MSG_ERROR ||
        message->origin_id > 1 || message->payload_len != expected_length ||
        (message->owner_id > 1 && message->owner_id != DSM_NO_PAGE) ||
        (message->type != MSG_HELLO && message->page_id >= DSM_PAGE_COUNT) ||
        (message->type == MSG_HELLO && message->page_id != DSM_NO_PAGE)) {
        errno = EPROTO;
        return DSM_ERROR;
    }
    return DSM_OK;
}

DSMStatus dsm_network_receive_message(int fd, DSMReceiveBuffer *buffer, DSMMessage *message)
{
    if (buffer->target == 0) buffer->target = DSM_HEADER_BYTES;
    for (;;) {
        if (buffer->used == DSM_HEADER_BYTES) {
            if (decode_header(buffer->data, message) != DSM_OK) return DSM_ERROR;
            buffer->target = DSM_HEADER_BYTES + message->payload_len;
        }
        if (buffer->used == buffer->target) {
            if (decode_header(buffer->data, message) != DSM_OK) return DSM_ERROR;
            memcpy(message->payload, buffer->data + DSM_HEADER_BYTES, message->payload_len);
            buffer->used = buffer->target = 0;
            return DSM_OK;
        }
        ssize_t count = recv(fd, buffer->data + buffer->used,
                             buffer->target - buffer->used, MSG_DONTWAIT);
        if (count < 0) {
            if (errno == EINTR) continue;
            return errno == EAGAIN || errno == EWOULDBLOCK ? DSM_AGAIN : DSM_ERROR;
        }
        if (count == 0) {
            if (buffer->used != 0) {
                errno = EPROTO;
                return DSM_ERROR;
            }
            return DSM_CLOSED;
        }
        buffer->used += (size_t)count;
    }
}
