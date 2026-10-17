#include "dsm/memory.h"
#include "dsm/cli.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    DSMMemory memory;
    unsigned char data[DSM_PAGE_BYTES];
    DSMCommand command;
    assert(dsm_memory_init(&memory, 1) == DSM_OK);
    assert(dsm_memory_read(&memory, 0, data) == DSM_ERROR);
    assert(dsm_memory_write(&memory, 0, "denied") == DSM_ERROR);
    memset(data, 0, sizeof(data));
    memcpy(data, "hola", 5);
    assert(dsm_memory_install(&memory, 255, data, 4, 0, 1) == DSM_OK);
    assert(memory.pages[255].state == PAGE_READ_ONLY);
    assert(dsm_memory_write(&memory, 255, "denied") == DSM_ERROR);
    assert(dsm_memory_install(&memory, 255, data, 4, 1, 1) == DSM_OK);
    assert(dsm_memory_write(&memory, 255, "nuevo") == DSM_OK);
    assert(memory.pages[255].version == 5);
    assert(dsm_memory_read(&memory, 255, data) == DSM_OK);
    assert(strcmp((const char *)data, "nuevo") == 0 && data[4095] == 0);
    assert(dsm_memory_invalidate(&memory, 255) == DSM_OK);
    assert(dsm_memory_read(&memory, 255, data) == DSM_ERROR);
    assert(dsm_memory_read(&memory, 256, data) == DSM_ERROR);
    assert(dsm_memory_invalidate(&memory, 256) == DSM_ERROR);
    memory.pages[0].state = PAGE_OWNER;
    memory.pages[0].version = UINT32_MAX;
    assert(dsm_memory_write(&memory, 0, "overflow") == DSM_ERROR);
    dsm_memory_destroy(&memory);

    assert(dsm_cli_parse("read 0", &command) == DSM_OK && command.type == CLI_READ);
    assert(dsm_cli_parse("read-copy 255", &command) == DSM_OK && command.type == CLI_COPY);
    assert(dsm_cli_parse("write 3 hola mundo", &command) == DSM_OK);
    assert(strcmp(command.text, "hola mundo") == 0);
    assert(dsm_cli_parse("status", &command) == DSM_OK && !command.has_page);
    assert(dsm_cli_parse("status 3", &command) == DSM_OK && command.has_page);
    const char *invalid[] = {"read -1", "read 256", "read 2junk", "read 0 more", "write 0",
        "read", "status -1", "exit extra", "write 99999999999999999999999 x", "unknown"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        assert(dsm_cli_parse(invalid[i], &command) == DSM_ERROR);
    }
    char line[DSM_PAGE_BYTES + 16];
    memcpy(line, "write 0 ", 8);
    memset(line + 8, 'x', 4095);
    line[4103] = 0;
    assert(dsm_cli_parse(line, &command) == DSM_OK);
    line[4103] = 'x'; line[4104] = 0;
    assert(dsm_cli_parse(line, &command) == DSM_ERROR);
    puts("PASS memory/cli: límites, permisos, versiones, parser y longitud máxima");
    return 0;
}
