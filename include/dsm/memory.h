#ifndef DSM_MEMORY_H
#define DSM_MEMORY_H

#include "dsm/types.h"

typedef enum { PAGE_INVALID, PAGE_READ_ONLY, PAGE_OWNER } DSMPageState;
typedef struct {
    DSMPageState state;
    uint32_t version;
    uint32_t owner_hint;
} DSMPage;
typedef struct {
    unsigned char *bytes;
    DSMPage pages[DSM_PAGE_COUNT];
} DSMMemory;

DSMStatus dsm_memory_init(DSMMemory *memory, int node_id);
void dsm_memory_destroy(DSMMemory *memory);
DSMStatus dsm_memory_read(const DSMMemory *memory, uint32_t page,
                          unsigned char output[DSM_PAGE_BYTES]);
DSMStatus dsm_memory_install(DSMMemory *memory, uint32_t page,
                             const unsigned char *data, uint32_t version,
                             uint32_t owner, int node_id);
DSMStatus dsm_memory_invalidate(DSMMemory *memory, uint32_t page);
/* Called only after protocol authorizes exclusive access. */
DSMStatus dsm_memory_write(DSMMemory *memory, uint32_t page, const char *text);
const char *dsm_memory_state_name(DSMPageState state);

#endif
