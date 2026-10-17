#include "dsm/network.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int pair[2];
    DSMLineBuffer line = {0};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    assert(dsm_network_receive(pair[1], &line) == DSM_AGAIN);
    assert(dsm_network_send(pair[0], "HEL") == DSM_OK);
    assert(dsm_network_receive(pair[1], &line) == DSM_AGAIN);
    assert(line.used == 3);
    assert(dsm_network_send(pair[0], "LO 0\nHELLO_ACK 1\n") == DSM_OK);
    assert(dsm_network_receive(pair[1], &line) == DSM_OK);
    assert(strcmp(line.data, "HELLO 0") == 0);
    line.used = 0;
    assert(dsm_network_receive(pair[1], &line) == DSM_OK);
    assert(strcmp(line.data, "HELLO_ACK 1") == 0);
    line.used = 0;
    dsm_network_close(&pair[0]);
    assert(pair[0] == -1);
    assert(dsm_network_receive(pair[1], &line) == DSM_CLOSED);
    assert(dsm_network_send(pair[1], "closed\n") == DSM_ERROR);
    assert(errno == EPIPE); /* MSG_NOSIGNAL must prevent process termination. */
    dsm_network_close(&pair[1]);
    dsm_network_close(&pair[1]);

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    assert(dsm_network_send(pair[0], "incomplete") == DSM_OK);
    dsm_network_close(&pair[0]);
    assert(dsm_network_receive(pair[1], &line) == DSM_ERROR);
    assert(errno == EPROTO);
    dsm_network_close(&pair[1]);

    line.used = 0;
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    char oversized[DSM_LINE_CAPACITY + 2];
    memset(oversized, 'x', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    assert(dsm_network_send(pair[0], oversized) == DSM_OK);
    assert(dsm_network_receive(pair[1], &line) == DSM_ERROR);
    assert(errno == EMSGSIZE);
    dsm_network_close(&pair[0]);
    dsm_network_close(&pair[1]);

    /* Force send() to span a small socket buffer while another process drains
     * a larger payload. Verify every byte, not the number of send() calls. */
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    int small_buffer = 1024;
    assert(setsockopt(pair[0], SOL_SOCKET, SO_SNDBUF,
                      &small_buffer, sizeof(small_buffer)) == 0);
    char payload[65537];
    memset(payload, 'a', sizeof(payload) - 1);
    payload[sizeof(payload) - 1] = '\0';
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(pair[0]);
        size_t total = 0;
        char chunk[137];
        ssize_t count;
        while ((count = read(pair[1], chunk, sizeof(chunk))) > 0) {
            for (ssize_t i = 0; i < count; ++i) {
                assert(chunk[i] == 'a');
            }
            total += (size_t)count;
        }
        assert(count == 0 && total == sizeof(payload) - 1);
        close(pair[1]);
        _exit(0);
    }
    dsm_network_close(&pair[1]);
    assert(dsm_network_send(pair[0], payload) == DSM_OK);
    dsm_network_close(&pair[0]);
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    puts("PASS network: fragmentación, líneas juntas, EOF, límites, SIGPIPE y envío completo");
    return 0;
}
