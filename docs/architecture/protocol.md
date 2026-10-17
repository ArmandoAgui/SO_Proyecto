# Protocolo implementado, versión 1

N0 coordina un acceso global activo y mantiene el directorio. Los datos pueden ser
propiedad de cualquiera. Todos los accesos, incluidos hits, solicitan autorización.
Las solicitudes de N0 usan el mismo coordinador mediante llamadas locales.

## Arranque y formato

Primero se conserva el saludo textual: N0 envía `HELLO 0\n`, N1 responde
`HELLO_ACK 1\n`. Después ambos envían HELLO_CONFIG binario con tres uint32 en orden
de red: node_id, page_count=256 y page_bytes=4096. Solo entonces aparece DSM_READY.

Cada trama binaria lleva 40 bytes, diez uint32 big endian:

| Offset | Campo |
|---|---|
| 0 | magic = 0x44534D31 (DSM1) |
| 4 | protocol_version = 1 |
| 8 | type |
| 12 | origin_id: solicitante original, no necesariamente emisor físico |
| 16 | request_id: creciente por origin |
| 20 | page_id; UINT32_MAX para HELLO_CONFIG |
| 24 | owner_id; UINT32_MAX si no hay copia/valor aplicable |
| 28 | page_version |
| 32 | flags: modo, fase ACK o código de error según tipo |
| 36 | payload_len |

Validaciones: magic/versión, tipo conocido, origen 0/1, propietario 0/1 o sentinel,
página en rango, tamaño exacto según tipo, roles, orden y correlación de respuestas.
No se envían enums, punteros, padding ni sizeof(struct). La cabecera se valida antes
de recibir el cuerpo, sin asignaciones basadas en longitud arbitraria del peer.

## Tipos y constantes

| Valor | Tipo | Payload | Uso |
|---|---|---|---|
| 1 | HELLO_CONFIG | 12 bytes | Negociar identidad/geometría; request=0. |
| 2 | READ_REQUEST | 0 | N1 solicita read o read-copy a N0. |
| 3 | WRITE_REQUEST | 0 | N1 solicita escritura sin ser dueño local. |
| 4 | FETCH_PAGE | 0 | N0 recupera bytes del dueño N1. |
| 5 | PAGE_DATA | 4096 bytes | Respuesta FETCH o instalación autorizada. |
| 6 | GRANT | 0 | Permiso cuando solicitante ya tiene datos vigentes. |
| 7 | INVALIDATE | 0 | N0 ordena invalidar N1; o N1 dueño pide invalidación/exclusividad a N0. |
| 8 | ACK | 0 | Confirmación de invalidación o finalización. |
| 9 | ERROR | 0 | BUSY o VERSION_EXHAUSTED; solicitud no ejecutada. |

Modos: ACCESS_READ=1 (migra), ACCESS_COPY=2 (replica), ACCESS_WRITE=3.
PAGE_DATA con flags=0 es FETCH_REPLY y se permite solo mientras N0 espera esos bytes.
ACK_INVALIDATED=1, ACK_DONE=2. ERROR_BUSY=1, ERROR_VERSION=2.
En solicitudes, owner_id=UINT32_MAX declara copia inválida; en caso contrario es hint,
y page_version declara la versión local. El directorio decide si hacen falta datos.

INVALIDATE tiene dos direcciones explícitas según rol: N0 → N1 es una orden cuyo ACK
es obligatorio; N1 → N0 inicia una transacción WRITE del dueño y N1 espera GRANT.
N0 aplica la invalidación local antes de esa concesión, que confirma exclusividad.
Una petición vieja que aún declare propiedad no concede nada por sí misma: N0 consulta
su directorio y recupera datos si ahora son necesarios.

## Máquina de transacción

```text
IDLE -> [FETCHING si faltan datos del dueño remoto]
     -> INVALIDATING -> espera ACK remotos
     -> WAIT_DONE -> ACK_DONE -> COMMIT -> IDLE
```

1. N0 admite solicitud o responde BUSY. Una WRITE con versión UINT32_MAX se rechaza.
2. Si faltan bytes al solicitante, toma instantánea del dueño, localmente o FETCH.
3. En READ/WRITE invalida todos los holders distintos del solicitante. En COPY no invalida.
4. Espera ACK_INVALIDATED de holders remotos; la invalidación propia se aplica localmente.
5. Envía PAGE_DATA si faltaban datos o GRANT si la copia ya era válida. Fija WAIT_DONE
   antes del despacho local, porque una operación N0 puede completarse inmediatamente.
6. Solicitante instala/promueve estado y ejecuta. WRITE aumenta versión; READ/COPY no.
7. Solicitante envía ACK_DONE. N0 valida versión/owner, actualiza directorio y libera tx.

En reposo READ y WRITE dejan valid_mask con solo el solicitante; COPY añade el lector
sin cambiar dueño. Las respuestas correlacionan origin, request, page y fase esperada.
Un ACK inesperado no desbloquea la transacción y provoca aborto de sesión.

## Flujos de demo

Migración inicial N0 → N1:

```text
N1 -> READ_REQUEST -> N0
N0: snapshot, INVALIDATE_APPLIED local
N0 -> PAGE_DATA OWNER -> N1
N1: instala, lee -> ACK_DONE -> N0 COMMIT owner=1
```

Copia de lectura N1 → N0, sin migrar:

```text
N0 -> FETCH_PAGE -> N1 -> PAGE_DATA FETCH_REPLY -> N0
N0: instala READ_ONLY, lee y confirma localmente; owner sigue 1
```

Escritura del dueño N1 con lector N0:

```text
N1 -> INVALIDATE (solicitud WRITE) -> N0
N0: INVALIDATE_APPLIED local -> GRANT WRITE -> N1
N1: escribe -> ACK_DONE -> N0 COMMIT nueva versión
```

Escritura N0 con lector N1:

```text
N0 -> INVALIDATE -> N1: INVALIDATE_APPLIED -> ACK_INVALIDATED -> N0
N0: GRANT_WRITE local, escritura y COMMIT
```

## Consistencia y fallos

Toda operación admitida se serializa en N0 hasta ACK_DONE. Los comandos completados
pueden ordenarse por esos commits, manteniendo el orden de cada nodo. Esto sustenta
la consistencia secuencial del modelo explícito bajo sesión sin fallos, sin afirmar
que las pruebas sean una demostración formal ni que accesos por punteros estén cubiertos.

Timeout absoluto: saludo/config 10 s; operación/trama parcial/cola pendiente 5 s.
EOF normal en reposo cierra; EOF parcial, tamaño incorrecto, timeout o fase incoherente
abortan. No hay rollback, persistencia ni reconexión. BUSY es un rechazo de operación,
no un error de transporte; requiere nueva solicitud con nuevo identificador.

Fuentes: [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html),
[recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html),
[send(2)](https://man7.org/linux/man-pages/man2/send.2.html).
