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
