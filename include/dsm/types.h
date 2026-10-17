#ifndef DSM_TYPES_H
#define DSM_TYPES_H

/* Transport/lifecycle results; no DSM state machine is implemented yet. */
typedef enum {
    DSM_OK = 0,
    DSM_NOT_IMPLEMENTED = 1,
    DSM_ERROR,
    DSM_CLOSED,
    DSM_AGAIN
} DSMStatus;

#endif /* DSM_TYPES_H */
