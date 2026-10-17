#include "dsm/protocol.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { DSMMessage messages[16]; size_t used; } Capture;
static DSMStatus capture(void *context, const DSMMessage *message)
{
    Capture *output = context;
    assert(output->used < 16);
    output->messages[output->used++] = *message;
    return DSM_OK;
}

int main(void)
{
    DSMProtocol *p = calloc(1, sizeof(*p));
    Capture *output = calloc(1, sizeof(*output));
    assert(p && output);
    assert(dsm_protocol_init(p, 0, capture, output) == DSM_OK);
    p->peer_ready = 1;
    /* A reader holds a copy. A write must wait for that reader's ACK. */
    p->directory[3].valid_mask = 3;
    DSMCommand write = {.type = CLI_WRITE, .page = 3};
    strcpy(write.text, "nuevo");
    assert(dsm_protocol_command(p, &write) == DSM_OK);
    assert(p->transaction.phase == TX_INVALIDATING);
    assert(output->used == 1 && output->messages[0].type == MSG_INVALIDATE);
    assert(p->memory.pages[3].version == 0);
    assert(p->memory.bytes[3 * DSM_PAGE_BYTES] == 0);
    DSMMessage ack = output->messages[0];
    ack.type = MSG_ACK;
    ack.flags = ACK_INVALIDATED;
    ack.page_id = 4;
    assert(dsm_protocol_on_message(p, &ack) == DSM_ERROR);
    assert(p->transaction.phase == TX_INVALIDATING);
    ack.page_id = 3;
    ack.page_version = 99;
    assert(dsm_protocol_on_message(p, &ack) == DSM_ERROR);
    assert(p->memory.pages[3].version == 0);
    ack.page_version = 0;
    assert(dsm_protocol_on_message(p, &ack) == DSM_OK);
    assert(p->transaction.phase == TX_IDLE);
    assert(p->directory[3].valid_mask == 1);
    assert(p->directory[3].version == 1);
    assert(strcmp((char *)p->memory.bytes + 3 * DSM_PAGE_BYTES, "nuevo") == 0);

    /* A pending global transaction rejects a simultaneous request as BUSY. */
    p->directory[4].valid_mask = 3;
    write.page = 4;
    assert(dsm_protocol_command(p, &write) == DSM_OK);
    DSMMessage request = {.type = MSG_READ_REQUEST, .origin_id = 1,
        .request_id = 1, .page_id = 7, .owner_id = DSM_NO_PAGE, .flags = ACCESS_READ};
    assert(dsm_protocol_on_message(p, &request) == DSM_OK);
    assert(output->messages[output->used - 1].type == MSG_ERROR);
    assert(output->messages[output->used - 1].flags == ERROR_BUSY);
    assert(p->transaction.page == 4 && p->memory.pages[7].state == PAGE_OWNER);
    assert(dsm_protocol_expired(p, p->transaction.started_ms + DSM_OPERATION_TIMEOUT_MS));
    dsm_protocol_destroy(p);
    free(p); free(output);
    puts("PASS protocol: ACK correcto antes de escribir, ACK falso rechazado, BUSY y timeout");
    return 0;
}
