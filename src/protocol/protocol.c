#define _POSIX_C_SOURCE 200809L
#include "dsm/protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int64_t dsm_monotonic_ms(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) return -1;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

const char *dsm_protocol_message_name(uint32_t type)
{
    static const char *const names[] = {"UNKNOWN", "HELLO_CONFIG", "READ_REQUEST",
        "WRITE_REQUEST", "FETCH_PAGE", "PAGE_DATA", "GRANT", "INVALIDATE", "ACK", "ERROR"};
    return type <= MSG_ERROR ? names[type] : names[0];
}

static DSMStatus protocol_error(DSMProtocol *p, const char *reason)
{
    fprintf(stderr, "[NODE %d] Error de protocolo: %s\n", p->id, reason);
    errno = EPROTO;
    return DSM_ERROR;
}

static DSMMessage make_message(uint32_t type, uint32_t origin, uint32_t request,
                               uint32_t page, uint32_t owner, uint32_t version, uint32_t flags)
{
    DSMMessage message = {0};
    message.type = type;
    message.origin_id = origin;
    message.request_id = request;
    message.page_id = page;
    message.owner_id = owner;
    message.page_version = version;
    message.flags = flags;
    return message;
}

static DSMStatus send_message(DSMProtocol *p, const DSMMessage *message)
{
    if (message->type == MSG_PAGE_DATA) p->pages_sent++;
    return p->send(p->send_context, message);
}

DSMStatus dsm_protocol_init(DSMProtocol *p, int id, DSMSendMessage send, void *context)
{
    memset(p, 0, sizeof(*p));
    p->id = id;
    p->send = send;
    p->send_context = context;
    for (size_t page = 0; page < DSM_PAGE_COUNT; ++page) {
        p->directory[page] = (DSMDirectoryEntry){0, 0, 1};
    }
    return dsm_memory_init(&p->memory, id);
}

void dsm_protocol_destroy(DSMProtocol *p)
{
    dsm_memory_destroy(&p->memory);
}

DSMStatus dsm_protocol_start(DSMProtocol *p)
{
    DSMMessage hello = make_message(MSG_HELLO, (uint32_t)p->id, 0, DSM_NO_PAGE,
                                    DSM_NO_PAGE, 0, 0);
    uint32_t config[3] = {htonl((uint32_t)p->id), htonl(DSM_PAGE_COUNT), htonl(DSM_PAGE_BYTES)};
    memcpy(hello.payload, config, sizeof(config));
    hello.payload_len = sizeof(config);
    return send_message(p, &hello);
}

static DSMStatus client_response(DSMProtocol *p, const DSMMessage *message);
static DSMStatus coordinator_ack(DSMProtocol *p, const DSMMessage *message, uint32_t sender);

static DSMStatus deliver(DSMProtocol *p, uint32_t target, const DSMMessage *message)
{
    return target == (uint32_t)p->id ? client_response(p, message) : send_message(p, message);
}

static void invalidate_local(DSMProtocol *p, uint32_t page, uint32_t request)
{
    dsm_memory_invalidate(&p->memory, page);
    p->invalidations++;
    printf("[NODE %d] req=%u page=%u INVALIDATE_APPLIED state=INVALID\n", p->id, request, page);
}

static DSMStatus grant_access(DSMProtocol *p)
{
    DSMTransaction *tx = &p->transaction;
    uint32_t owner = tx->mode == ACCESS_COPY ? tx->old_owner : tx->origin;
    DSMMessage grant = make_message(tx->needs_data ? MSG_PAGE_DATA : MSG_GRANT,
        tx->origin, tx->request, tx->page, owner, tx->version, tx->mode);
    if (tx->needs_data) {
        grant.payload_len = DSM_PAGE_BYTES;
        memcpy(grant.payload, tx->snapshot, DSM_PAGE_BYTES);
    }
    tx->phase = TX_WAIT_DONE; /* Set before local delivery, which may ACK immediately. */
    printf("[NODE 0] req=%u page=%u GRANT_%s owner=%u invalidations_confirmed=1\n",
           tx->request, tx->page, tx->mode == ACCESS_WRITE ? "WRITE" : "READ", owner);
    return deliver(p, tx->origin, &grant);
}

static DSMStatus prepare_access(DSMProtocol *p)
{
    DSMTransaction *tx = &p->transaction;
    tx->phase = TX_INVALIDATING;
    tx->invalidating = tx->mode == ACCESS_COPY ? 0 :
        p->directory[tx->page].valid_mask & ~(1u << tx->origin);
    if (tx->invalidating & 1u) {
        invalidate_local(p, tx->page, tx->request);
        tx->invalidating &= ~1u;
    }
    if (tx->invalidating & 2u) {
        DSMMessage invalidate = make_message(MSG_INVALIDATE, tx->origin, tx->request,
            tx->page, tx->old_owner, tx->version, tx->mode);
        if (send_message(p, &invalidate) != DSM_OK) return DSM_ERROR;
    }
    return tx->invalidating == 0 ? grant_access(p) : DSM_OK;
}

static DSMStatus coordinator_request(DSMProtocol *p, const DSMMessage *request)
{
    uint32_t origin = request->origin_id;
    if (origin > 1 || request->request_id == 0 || request->request_id <= p->last_request[origin] ||
        request->page_id >= DSM_PAGE_COUNT || request->payload_len != 0 ||
        (request->type == MSG_READ_REQUEST && request->flags != ACCESS_READ && request->flags != ACCESS_COPY) ||
        (request->type == MSG_WRITE_REQUEST && request->flags != ACCESS_WRITE)) {
        return protocol_error(p, "solicitud inválida o identificador repetido");
    }
    p->last_request[origin] = request->request_id;
    DSMDirectoryEntry *entry = &p->directory[request->page_id];
    if (p->transaction.phase != TX_IDLE || (request->flags == ACCESS_WRITE && entry->version == UINT32_MAX)) {
        DSMMessage error = *request;
        error.type = MSG_ERROR;
        error.flags = p->transaction.phase != TX_IDLE ? ERROR_BUSY : ERROR_VERSION;
        printf("[NODE 0] req=%u page=%u REJECT %s\n", request->request_id, request->page_id,
               error.flags == ERROR_BUSY ? "BUSY" : "VERSION_EXHAUSTED");
        return deliver(p, origin, &error);
    }
    DSMTransaction *tx = &p->transaction;
    memset(tx, 0, sizeof(*tx));
    tx->origin = origin;
    tx->request = request->request_id;
    tx->page = request->page_id;
    tx->mode = request->flags;
    tx->old_owner = entry->owner;
    tx->version = entry->version;
    tx->started_ms = dsm_monotonic_ms();
    tx->needs_data = !(entry->valid_mask & (1u << origin)) ||
                     request->owner_id == DSM_NO_PAGE || request->page_version != entry->version;
    if (tx->needs_data) {
        if (entry->owner == 0) {
            if (dsm_memory_read(&p->memory, tx->page, tx->snapshot) != DSM_OK) {
                return protocol_error(p, "directorio sin datos del propietario local");
            }
        } else {
            tx->phase = TX_FETCHING;
            DSMMessage fetch = make_message(MSG_FETCH_PAGE, origin, tx->request,
                                             tx->page, entry->owner, entry->version, 0);
            return send_message(p, &fetch);
        }
    }
    return prepare_access(p);
}

static int matches_transaction(const DSMTransaction *tx, const DSMMessage *message)
{
    return message->request_id == tx->request && message->origin_id == tx->origin &&
           message->page_id == tx->page;
}

static DSMStatus coordinator_ack(DSMProtocol *p, const DSMMessage *message, uint32_t sender)
{
    DSMTransaction *tx = &p->transaction;
    if (!matches_transaction(tx, message)) return protocol_error(p, "ACK sin transacción correspondiente");
    if (message->flags == ACK_INVALIDATED && tx->phase == TX_INVALIDATING &&
        message->page_version == tx->version && (tx->invalidating & (1u << sender))) {
        tx->invalidating &= ~(1u << sender);
        return tx->invalidating == 0 ? grant_access(p) : DSM_OK;
    }
    uint32_t version = tx->version + (tx->mode == ACCESS_WRITE ? 1u : 0u);
    if (message->flags != ACK_DONE || tx->phase != TX_WAIT_DONE || sender != tx->origin ||
        message->page_version != version || message->owner_id !=
        (tx->mode == ACCESS_COPY ? tx->old_owner : tx->origin)) {
        return protocol_error(p, "ACK fuera de fase, versión o propietario incorrecto");
    }
    DSMDirectoryEntry *entry = &p->directory[tx->page];
    uint32_t old_owner = entry->owner;
    entry->version = version;
    entry->owner = message->owner_id;
    entry->valid_mask = tx->mode == ACCESS_COPY ? entry->valid_mask | (1u << tx->origin) : 1u << tx->origin;
    p->memory.pages[tx->page].owner_hint = entry->owner;
    printf("[NODE 0] req=%u page=%u COMMIT owner=%u version=%u valid_mask=%u\n",
           tx->request, tx->page, entry->owner, version, entry->valid_mask);
    if (old_owner != entry->owner) {
        printf("[NODE 0] req=%u page=%u OWNER_CHANGED %u -> %u\n",
               tx->request, tx->page, old_owner, entry->owner);
    }
    tx->phase = TX_IDLE;
    p->completed++;
    return DSM_OK;
}

static DSMStatus client_response(DSMProtocol *p, const DSMMessage *message)
{
    uint32_t page = message->page_id;
    if (page >= DSM_PAGE_COUNT) return protocol_error(p, "página fuera de rango");
    DSMPage *local = &p->memory.pages[page];
    if (!p->pending || message->origin_id != (uint32_t)p->id ||
        message->request_id != p->pending_request || page != p->pending_page) {
        return protocol_error(p, "respuesta sin petición local pendiente");
    }
    if (message->type == MSG_ERROR) {
        if (message->flags != ERROR_BUSY && message->flags != ERROR_VERSION) {
            return protocol_error(p, "código de error desconocido");
        }
        printf("[NODE %d] req=%u page=%u ERROR %s; reintente manualmente\n", p->id,
               message->request_id, page, message->flags == ERROR_BUSY ? "BUSY" : "VERSION_EXHAUSTED");
        p->pending = 0;
        return DSM_OK;
    }
    if ((message->type != MSG_GRANT && message->type != MSG_PAGE_DATA) ||
        message->flags != p->pending_mode || message->owner_id > 1 ||
        (message->flags != ACCESS_COPY && message->owner_id != (uint32_t)p->id)) {
        return protocol_error(p, "concesión inesperada");
    }
    if (message->type == MSG_PAGE_DATA) {
        if (message->payload_len != DSM_PAGE_BYTES) return protocol_error(p, "página incompleta");
        dsm_memory_install(&p->memory, page, message->payload, message->page_version,
                           message->owner_id, p->id);
        printf("[NODE %d] req=%u page=%u PAGE_INSTALLED bytes=%u state=%s\n", p->id,
            message->request_id, page, DSM_PAGE_BYTES, dsm_memory_state_name(local->state));
    } else {
        if (local->state == PAGE_INVALID || local->version != message->page_version) {
            return protocol_error(p, "concesión local sobre copia inválida/obsoleta");
        }
        local->state = message->owner_id == (uint32_t)p->id ? PAGE_OWNER : PAGE_READ_ONLY;
        local->owner_hint = message->owner_id;
    }
    if (p->pending_mode == ACCESS_WRITE && dsm_memory_write(&p->memory, page, p->pending_text) != DSM_OK) {
        return protocol_error(p, "escritura sin precondiciones válidas");
    }
    unsigned char data[DSM_PAGE_BYTES];
    if (dsm_memory_read(&p->memory, page, data) != DSM_OK) return protocol_error(p, "lectura inválida");
    printf("[NODE %d] RESULT %s req=%u page=%u version=%u data=", p->id,
           p->pending_mode == ACCESS_WRITE ? "WRITE" : p->pending_mode == ACCESS_COPY ? "READ_COPY" : "READ",
           message->request_id, page, local->version);
    dsm_cli_print_data(data, sizeof(data));
    putchar('\n');
    DSMMessage done = make_message(MSG_ACK, (uint32_t)p->id, p->pending_request, page,
                                    local->owner_hint, local->version, ACK_DONE);
    p->pending = 0;
    return p->id == 0 ? coordinator_ack(p, &done, 0) : send_message(p, &done);
}

DSMStatus dsm_protocol_on_message(DSMProtocol *p, const DSMMessage *m)
{
    if (m->type == MSG_HELLO) {
        uint32_t config[3];
        if (p->peer_ready || m->payload_len != sizeof(config) || m->request_id != 0 ||
            m->origin_id != (uint32_t)(1 - p->id) || m->flags != 0) {
            return protocol_error(p, "HELLO_CONFIG inesperado");
        }
        memcpy(config, m->payload, sizeof(config));
        if (ntohl(config[0]) != m->origin_id || ntohl(config[1]) != DSM_PAGE_COUNT || ntohl(config[2]) != DSM_PAGE_BYTES) {
            return protocol_error(p, "geometría/identidad incompatible");
        }
        p->peer_ready = 1;
        printf("[NODE %d] DSM_READY pages=%u page_bytes=%u\n", p->id, DSM_PAGE_COUNT, DSM_PAGE_BYTES);
        dsm_cli_print_help();
        return DSM_OK;
    }
    if (!p->peer_ready || m->page_id >= DSM_PAGE_COUNT || m->origin_id > 1 || m->request_id == 0) {
        return protocol_error(p, "mensaje antes de HELLO o campos inválidos");
    }
    if (m->type == MSG_PAGE_DATA) p->pages_received++;
    if (p->id == 0) {
        if (m->type == MSG_READ_REQUEST || m->type == MSG_WRITE_REQUEST || m->type == MSG_INVALIDATE) {
            if (m->origin_id != 1) return protocol_error(p, "identidad de solicitante incorrecta");
            if (m->type == MSG_INVALIDATE) {
                /* An owner asks the coordinator to invalidate its other copy
                 * before obtaining exclusive write permission. */
                if (m->flags != ACCESS_WRITE || m->owner_id != 1) {
                    return protocol_error(p, "petición de invalidación inválida");
                }
                DSMMessage request = *m;
                request.type = MSG_WRITE_REQUEST;
                return coordinator_request(p, &request);
            }
            return coordinator_request(p, m);
        }
        if (m->type == MSG_ACK) return coordinator_ack(p, m, 1);
        DSMTransaction *tx = &p->transaction;
        if (m->type == MSG_PAGE_DATA && tx->phase == TX_FETCHING && matches_transaction(tx, m) &&
            m->flags == 0 && m->owner_id == 1 && m->page_version == tx->version && m->payload_len == DSM_PAGE_BYTES) {
            memcpy(tx->snapshot, m->payload, DSM_PAGE_BYTES);
            return prepare_access(p);
        }
        return protocol_error(p, "mensaje no permitido para coordinador/fase");
    }
    if (m->type == MSG_FETCH_PAGE) {
        DSMPage *local = &p->memory.pages[m->page_id];
        DSMMessage data = *m;
        if (m->owner_id != 1 || m->flags != 0 || local->state != PAGE_OWNER ||
            local->version != m->page_version || dsm_memory_read(&p->memory, m->page_id, data.payload) != DSM_OK) {
            return protocol_error(p, "FETCH sin propiedad válida");
        }
        data.type = MSG_PAGE_DATA;
        data.payload_len = DSM_PAGE_BYTES;
        return send_message(p, &data);
    }
    if (m->type == MSG_INVALIDATE) {
        DSMPage *local = &p->memory.pages[m->page_id];
        if (m->origin_id != 0 || (m->flags != ACCESS_READ && m->flags != ACCESS_WRITE) ||
            local->state == PAGE_INVALID || local->version != m->page_version) {
            return protocol_error(p, "INVALIDATE inesperado/obsoleto");
        }
        invalidate_local(p, m->page_id, m->request_id);
        DSMMessage ack = *m;
        ack.type = MSG_ACK;
        ack.flags = ACK_INVALIDATED;
        return send_message(p, &ack);
    }
    return client_response(p, m);
}

DSMStatus dsm_protocol_command(DSMProtocol *p, const DSMCommand *command)
{
    if (p->pending) {
        printf("[NODE %d] BUSY local: espere el resultado de req=%u\n", p->id, p->pending_request);
        return DSM_OK;
    }
    if (p->next_request == UINT32_MAX) return protocol_error(p, "identificadores agotados; reinicie sesión");
    p->pending = 1;
    p->pending_request = ++p->next_request;
    p->pending_page = command->page;
    p->pending_mode = command->type == CLI_WRITE ? ACCESS_WRITE : command->type == CLI_COPY ? ACCESS_COPY : ACCESS_READ;
    p->pending_started_ms = dsm_monotonic_ms();
    memcpy(p->pending_text, command->text, sizeof(p->pending_text));
    DSMPage *local = &p->memory.pages[command->page];
    if (local->state == PAGE_INVALID) {
        p->faults++;
        printf("[NODE %d] req=%u page=%u PAGE_FAULT simulated=1\n", p->id, p->pending_request, command->page);
    } else {
        printf("[NODE %d] req=%u page=%u LOCAL_HIT authorization_required=1\n", p->id, p->pending_request, command->page);
    }
    DSMMessage request = make_message(p->pending_mode == ACCESS_WRITE ? MSG_WRITE_REQUEST : MSG_READ_REQUEST,
        (uint32_t)p->id, p->pending_request, command->page,
        local->state == PAGE_INVALID ? DSM_NO_PAGE : local->owner_hint, local->version, p->pending_mode);
    if (p->id == 1 && p->pending_mode == ACCESS_WRITE && local->state == PAGE_OWNER) {
        request.type = MSG_INVALIDATE;
    }
    return p->id == 0 ? coordinator_request(p, &request) : send_message(p, &request);
}

static void print_page(const DSMProtocol *p, uint32_t page)
{
    const DSMPage *local = &p->memory.pages[page];
    printf("[NODE %d] PAGE page=%u state=%s version=%u owner_hint=%u", p->id, page,
           dsm_memory_state_name(local->state), local->version, local->owner_hint);
    if (p->id == 0) printf(" directory_owner=%u directory_version=%u valid_mask=%u",
        p->directory[page].owner, p->directory[page].version, p->directory[page].valid_mask);
    putchar('\n');
}

void dsm_protocol_status(const DSMProtocol *p, int has_page, uint32_t page)
{
    printf("[NODE %d] STATUS pending=%d transaction=%d faults=%" PRIu64
           " pages_sent=%" PRIu64 " pages_received=%" PRIu64 " invalidations=%" PRIu64
           " commits=%" PRIu64 "\n", p->id, p->pending, p->transaction.phase,
           p->faults, p->pages_sent, p->pages_received, p->invalidations, p->completed);
    if (has_page) {
        print_page(p, page);
    } else {
        unsigned int counts[3] = {0};
        int printed = 0;
        for (uint32_t i = 0; i < DSM_PAGE_COUNT; ++i) {
            const DSMPage *local = &p->memory.pages[i];
            counts[local->state]++;
            if (local->version || local->owner_hint || (p->id == 0 && local->state != PAGE_OWNER) ||
                (p->id == 1 && local->state != PAGE_INVALID)) {
                print_page(p, i);
                printed = 1;
            }
        }
        printf("[NODE %d] MEMORY bytes=%u INVALID=%u READ_ONLY=%u OWNER=%u\n", p->id,
               DSM_PAGE_COUNT * DSM_PAGE_BYTES, counts[0], counts[1], counts[2]);
        if (!printed) print_page(p, 0);
    }
}

int dsm_protocol_busy(const DSMProtocol *p)
{
    return p->pending || p->transaction.phase != TX_IDLE;
}

int dsm_protocol_expired(const DSMProtocol *p, int64_t now_ms)
{
    return (p->pending && now_ms - p->pending_started_ms >= DSM_OPERATION_TIMEOUT_MS) ||
           (p->transaction.phase != TX_IDLE && now_ms - p->transaction.started_ms >= DSM_OPERATION_TIMEOUT_MS);
}
