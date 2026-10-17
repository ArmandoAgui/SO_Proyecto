#include "dsm/cli.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DSMStatus dsm_cli_parse(const char *line, DSMCommand *command)
{
    memset(command, 0, sizeof(*command));
    while (isspace((unsigned char)*line)) ++line;
    if (*line == '\0') return DSM_OK;
    const char *end = line;
    while (*end && !isspace((unsigned char)*end)) ++end;
    size_t length = (size_t)(end - line);
    if (length == 4 && strncmp(line, "read", 4) == 0) command->type = CLI_READ;
    else if (length == 9 && strncmp(line, "read-copy", 9) == 0) command->type = CLI_COPY;
    else if (length == 5 && strncmp(line, "write", 5) == 0) command->type = CLI_WRITE;
    else if (length == 6 && strncmp(line, "status", 6) == 0) command->type = CLI_STATUS;
    else if (length == 4 && strncmp(line, "help", 4) == 0) command->type = CLI_HELP;
    else if (length == 4 && strncmp(line, "exit", 4) == 0) command->type = CLI_EXIT;
    else return DSM_ERROR;
    while (isspace((unsigned char)*end)) ++end;
    if (command->type == CLI_HELP || command->type == CLI_EXIT) {
        return *end ? DSM_ERROR : DSM_OK;
    }
    if (command->type == CLI_STATUS && *end == '\0') return DSM_OK;
    if (!isdigit((unsigned char)*end)) return DSM_ERROR;
    errno = 0;
    char *number_end;
    unsigned long page = strtoul(end, &number_end, 10);
    if (errno || page >= DSM_PAGE_COUNT || (*number_end && !isspace((unsigned char)*number_end))) {
        return DSM_ERROR;
    }
    command->page = (uint32_t)page;
    command->has_page = 1;
    end = number_end;
    while (isspace((unsigned char)*end)) ++end;
    if (command->type != CLI_WRITE) return *end ? DSM_ERROR : DSM_OK;
    length = strlen(end);
    if (length == 0 || length >= DSM_PAGE_BYTES) return DSM_ERROR;
    memcpy(command->text, end, length + 1);
    return DSM_OK;
}

void dsm_cli_print_help(void)
{
    puts("Comandos: read <0..255>             Migra la propiedad para leer");
    puts("          read-copy <0..255>        Obtiene/conserva copia de solo lectura");
    puts("          write <0..255> <texto>    Exclusividad y reemplazo (1..4095 bytes)");
    puts("          status [0..255]           Estado local y directorio en N0");
    puts("          help | exit               Ayuda / cierre ordenado");
    puts("Texto literal: sin interpretar comillas ni comandos de shell. Enter cierra.");
}

void dsm_cli_print_data(const unsigned char *data, size_t length)
{
    putchar('"');
    for (size_t i = 0; i < length && data[i] != '\0'; ++i) {
        unsigned char byte = data[i];
        if (byte == '"' || byte == '\\') printf("\\%c", byte);
        else if (byte >= 32 && byte <= 126) putchar(byte);
        else printf("\\x%02x", byte);
    }
    putchar('"');
}
