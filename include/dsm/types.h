#ifndef DSM_TYPES_H
#define DSM_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define DSM_PAGE_COUNT 256u
#define DSM_PAGE_BYTES 4096u
#define DSM_NO_PAGE UINT32_MAX
#define DSM_PROTOCOL_VERSION 1u
#define DSM_WIRE_MAGIC 0x44534d31u
#define DSM_HEADER_BYTES 40u

typedef enum {
    DSM_OK = 0, DSM_NOT_IMPLEMENTED, DSM_ERROR, DSM_CLOSED, DSM_AGAIN
} DSMStatus;

typedef enum {
    MSG_HELLO = 1, MSG_READ_REQUEST, MSG_WRITE_REQUEST, MSG_FETCH_PAGE,
    MSG_PAGE_DATA, MSG_GRANT, MSG_INVALIDATE, MSG_ACK, MSG_ERROR
} DSMMessageType;

typedef enum { ACCESS_READ = 1, ACCESS_COPY, ACCESS_WRITE } DSMAccess;
typedef enum { ACK_INVALIDATED = 1, ACK_DONE } DSMAck;
typedef enum { ERROR_BUSY = 1, ERROR_VERSION } DSMError;

typedef struct {
    uint32_t type, origin_id, request_id, page_id, owner_id, page_version;
    uint32_t flags, payload_len;
    unsigned char payload[DSM_PAGE_BYTES];
} DSMMessage;

#endif
