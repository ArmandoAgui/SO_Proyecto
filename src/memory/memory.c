#include "dsm/memory.h"
#include <stdlib.h>
#include <string.h>

DSMStatus dsm_memory_init(DSMMemory *memory, int node_id)
{
    memset(memory, 0, sizeof(*memory));
    memory->bytes = calloc(DSM_PAGE_COUNT, DSM_PAGE_BYTES);
    if (memory->bytes == NULL) {
        return DSM_ERROR;
    }
    for (size_t p = 0; p < DSM_PAGE_COUNT; ++p) {
        memory->pages[p].state = node_id == 0 ? PAGE_OWNER : PAGE_INVALID;
        memory->pages[p].owner_hint = 0;
    }
    return DSM_OK;
}

void dsm_memory_destroy(DSMMemory *memory)
{
    free(memory->bytes);
    memory->bytes = NULL;
}

DSMStatus dsm_memory_read(const DSMMemory *memory, uint32_t page,
                          unsigned char output[DSM_PAGE_BYTES])
{
    if (page >= DSM_PAGE_COUNT || memory->pages[page].state == PAGE_INVALID) {
        return DSM_ERROR;
    }
    memcpy(output, memory->bytes + (size_t)page * DSM_PAGE_BYTES, DSM_PAGE_BYTES);
    return DSM_OK;
}

DSMStatus dsm_memory_install(DSMMemory *memory, uint32_t page,
                             const unsigned char *data, uint32_t version,
                             uint32_t owner, int node_id)
{
    if (page >= DSM_PAGE_COUNT || owner > 1) {
        return DSM_ERROR;
    }
    memcpy(memory->bytes + (size_t)page * DSM_PAGE_BYTES, data, DSM_PAGE_BYTES);
    memory->pages[page] = (DSMPage){owner == (uint32_t)node_id ? PAGE_OWNER : PAGE_READ_ONLY,
                                    version, owner};
    return DSM_OK;
}

DSMStatus dsm_memory_invalidate(DSMMemory *memory, uint32_t page)
{
    if (page >= DSM_PAGE_COUNT) {
        return DSM_ERROR;
    }
    memory->pages[page].state = PAGE_INVALID;
    return DSM_OK;
}

DSMStatus dsm_memory_write(DSMMemory *memory, uint32_t page, const char *text)
{
    size_t length = strlen(text);
    if (page >= DSM_PAGE_COUNT || memory->pages[page].state != PAGE_OWNER ||
        memory->pages[page].version == UINT32_MAX || length == 0 || length >= DSM_PAGE_BYTES) {
        return DSM_ERROR;
    }
    unsigned char *destination = memory->bytes + (size_t)page * DSM_PAGE_BYTES;
    memset(destination, 0, DSM_PAGE_BYTES);
    memcpy(destination, text, length);
    memory->pages[page].version++;
    return DSM_OK;
}

const char *dsm_memory_state_name(DSMPageState state)
{
    switch (state) {
    case PAGE_INVALID: return "INVALID";
    case PAGE_READ_ONLY: return "READ_ONLY";
    case PAGE_OWNER: return "OWNER";
    }
    return "UNKNOWN";
}
