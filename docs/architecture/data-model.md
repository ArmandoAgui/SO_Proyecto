# Modelo de datos implementado

Las definiciones compilables están en include/dsm. Los tamaños de las estructuras C
no son el formato de red: la serialización escribe diez enteros uint32 explícitos.

| Tipo | Datos y propósito |
|---|---|
| DSMNode | ID, puerto local, peer-port, server_fd y peer_fd; configuración/recursos. |
| DSMMemory | Puntero a calloc(256,4096) y 256 metadatos DSMPage. |
| DSMPage | Estado INVALID/READ_ONLY/OWNER, versión y owner_hint informativo. |
| DSMDirectoryEntry | Propietario, versión confirmada y máscara de copias válidas. Solo N0 decide con ella. |
| DSMTransaction | Fase, origin/request, página, modo, dueño anterior, versión, máscara de ACK, plazo e instantánea. |
| DSMProtocol | Memoria, directorio, transacción, solicitud local pendiente y callback de envío. |
| DSMMessage | Tipo y campos lógicos más payload de hasta 4096 bytes; representación interna. |
| DSMReceiveBuffer | Cabecera/cuerpo en construcción, bytes acumulados y objetivo. |
| DSMSendBuffer | Cola de 65536 bytes, total y offset ya enviado. |
| DSMCommand | Comando, página opcional y texto de escritura. |

La página p comienza en `memory.bytes + p * DSM_PAGE_BYTES`. Se valida p antes de
acceder. Cada nodo reserva bytes para todas las páginas, pero no puede usar una que
esté INVALID. La invalidación no borra los bytes; los metadatos impiden usarlos.

Inicio: N0 OWNER en todo, versión 0 y datos cero. N1 INVALID en todo. Directorio N0:
owner=0, version=0, valid_mask=1. En la máscara bit 0=N0 y bit 1=N1.

La versión aumenta exactamente una vez al escribir. Un incremento desde UINT32_MAX
se rechaza. Los identificadores por origin son crecientes, no se reutilizan tras BUSY
y no pueden desbordarse. No hay LRU, swapping, dirty bit ni asignación dinámica de lectores.

La instantánea de la transacción NO es una copia válida añadida al directorio.
owner_hint puede estar desactualizado, especialmente en páginas INVALID; nunca concede
permisos. Durante una transacción, el directorio describe el último commit y puede
diferir temporalmente de los estados locales. status expone la fase para evitar
interpretar ese momento como un estado estable.
