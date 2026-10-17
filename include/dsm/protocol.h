#ifndef DSM_PROTOCOL_H
#define DSM_PROTOCOL_H
#include "dsm/cli.h"
#include "dsm/memory.h"

#define DSM_OPERATION_TIMEOUT_MS 5000

typedef struct {
    uint32_t owner, version, valid_mask;
} DSMDirectoryEntry;
typedef enum { TX_IDLE, TX_FETCHING, TX_INVALIDATING, TX_WAIT_DONE } DSMTransactionPhase;
typedef struct {
    DSMTransactionPhase phase;
    uint32_t origin, request, page, mode, old_owner, version, invalidating;
    int needs_data;
    int64_t started_ms;
    unsigned char snapshot[DSM_PAGE_BYTES];
} DSMTransaction;
typedef DSMStatus (*DSMSendMessage)(void *context, const DSMMessage *message);
typedef struct {
    int id, peer_ready, pending;
    uint32_t next_request, last_request[2], pending_request, pending_page, pending_mode;
    int64_t pending_started_ms;
    char pending_text[DSM_PAGE_BYTES];
    DSMMemory memory;
    DSMDirectoryEntry directory[DSM_PAGE_COUNT]; /* Authoritative only in N0. */
    DSMTransaction transaction;
    DSMSendMessage send;
    void *send_context;
    uint64_t faults, pages_sent, pages_received, invalidations, completed;
} DSMProtocol;

DSMStatus dsm_protocol_init(DSMProtocol *protocol, int id, DSMSendMessage send, void *context);
void dsm_protocol_destroy(DSMProtocol *protocol);
DSMStatus dsm_protocol_start(DSMProtocol *protocol);
DSMStatus dsm_protocol_on_message(DSMProtocol *protocol, const DSMMessage *message);
DSMStatus dsm_protocol_command(DSMProtocol *protocol, const DSMCommand *command);
void dsm_protocol_status(const DSMProtocol *protocol, int has_page, uint32_t page);
int dsm_protocol_busy(const DSMProtocol *protocol);
int dsm_protocol_expired(const DSMProtocol *protocol, int64_t now_ms);
int64_t dsm_monotonic_ms(void);
const char *dsm_protocol_message_name(uint32_t type);
#endif
