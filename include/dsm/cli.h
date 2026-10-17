#ifndef DSM_CLI_H
#define DSM_CLI_H
#include "dsm/types.h"

typedef enum { CLI_EMPTY, CLI_READ, CLI_COPY, CLI_WRITE, CLI_STATUS, CLI_HELP, CLI_EXIT } DSMCommandType;
typedef struct {
    DSMCommandType type;
    uint32_t page;
    int has_page;
    char text[DSM_PAGE_BYTES];
} DSMCommand;

DSMStatus dsm_cli_parse(const char *line, DSMCommand *command);
void dsm_cli_print_help(void);
/* Print untrusted page bytes as escaped text, bounded by one page. */
void dsm_cli_print_data(const unsigned char *data, size_t length);
#endif
