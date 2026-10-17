# Modelo de datos propuesto

La Fase 1 ya define un `DSMNode` mínimo en `include/dsm/node.h`: ID, puertos y
descriptores, sin memoria ni transacciones.

Los siguientes fragmentos son bocetos de contrato, no encabezados compilables ni una
implementación. Confirmar nombres antes de la fase 2. Usar `<stdint.h>`, `<stdbool.h>`
y `<stddef.h>` en los futuros encabezados.

```c
#define DSM_NODE_COUNT 2u
#define DSM_PAGE_COUNT 256u
#define DSM_PAGE_BYTES 4096u

typedef enum {
    PAGE_INVALID,
    PAGE_READ_ONLY,
    PAGE_OWNER
} PageState;

typedef struct {
    PageState state;
    uint32_t version;       /* versión de los bytes locales; obsoleta si INVALID */
    int owner_hint;         /* informativo; -1 si desconocido */
} Page;

typedef struct {
    unsigned char *bytes;  /* calloc(PAGE_COUNT, PAGE_BYTES), arreglo contiguo */
    Page pages[DSM_PAGE_COUNT];
} Memory;

typedef struct {
    uint32_t owner_id;
    uint32_t version;
    uint8_t valid_mask;    /* bit 0=N0 y bit 1=N1, incluye al dueño */
} DirectoryEntry;

typedef enum {
    MSG_HELLO = 1, MSG_READ_REQUEST, MSG_WRITE_REQUEST,
    MSG_FETCH_PAGE, MSG_PAGE_DATA, MSG_GRANT,
    MSG_INVALIDATE, MSG_ACK, MSG_ERROR
} MessageType;

typedef struct {
    MessageType type;
    uint32_t request_id;
    uint32_t page_id;
    uint32_t version;
    uint32_t flags;         /* modo READ/WRITE o fase ACK según type */
    uint32_t payload_len;
    unsigned char payload[DSM_PAGE_BYTES];
} Message;
```

`Page` solo tiene metadatos; los bytes de la página p se encuentran en
`memory.bytes + p * DSM_PAGE_BYTES`. Evitar otra copia completa dentro de cada Page.
Validar p antes de multiplicar y no leer bytes con estado INVALID.
`valid_mask` basta para dos nodos: no crear listas dinámicas de lectores.
No hacen falta dirty bit, LRU, frames físicos ni tabla de reemplazo: no hay evicción.
La versión aumenta al confirmar cada escritura, no al copiar ni invalidar.
Se rechaza una escritura que desbordaría el contador; la sesión se reinicia para
continuar. Los request_id son crecientes por emisor y no se reutilizan en una sesión;
se identifica una operación mediante (nodo solicitante, request_id).

```c
typedef enum {
    TX_IDLE, TX_FETCHING, TX_INVALIDATING,
    TX_WAIT_READ_DONE, TX_WAIT_WRITE_DONE
} TransactionPhase;

typedef struct {
    TransactionPhase phase;
    uint32_t requester_id, request_id, page_id;
    uint32_t old_owner, old_version;
    uint8_t pending_ack_mask;
    unsigned char snapshot[DSM_PAGE_BYTES];
    /* modo, indicador snapshot válido y plazo monotónico */
} Transaction;

typedef struct {
    uint32_t node_id;
    int peer_fd;
    Memory memory;
    DirectoryEntry *directory; /* tabla reservada solo en N0; NULL en N1 */
    Transaction transaction;  /* N0: transacción global; N1: petición local */
    bool running;
    /* configuración, buffers network, orden CLI pendiente y contadores */
} DSMNode;
```

`DSMNode` es contexto de recursos, no un módulo con toda la lógica. Configuración y
buffer de transporte tendrán tipos propios cuando se implemente network, sin anticipar
jerarquías genéricas. `Message` es una representación en memoria: sus enums, padding,
punteros y sizeof **no definen** el formato transmitido.

La instantánea permite recuperar los bytes antes de invalidar al dueño anterior.
Mientras la transacción está activa el directorio describe el último estado confirmado
más su transacción pendiente; no puede usarse para servir otro acceso. No se promete
que los estados intermedios tengan exactamente un OWNER; la invariante aplica al
estado estable. Una desconexión en transferencia detiene la sesión.
