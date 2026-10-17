#ifndef DSM_NODE_H
#define DSM_NODE_H

#include "dsm/types.h"

typedef struct {
    int id;
    int listen_port;
    int peer_port;
    int server_fd;
    int peer_fd;
} DSMNode;

/* Configuration is validated by main; descriptors start at -1. */
DSMStatus dsm_node_run(DSMNode *node);

#endif /* DSM_NODE_H */
