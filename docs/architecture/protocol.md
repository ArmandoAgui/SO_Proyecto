# Protocolo conceptual — propuesta, sin implementar

## Autoridad y orden

N0 mantiene el directorio y admite una sola operación global. Todas las órdenes read/write
se despachan allí; las originadas en N0 siguen la misma máquina de estados mediante
llamadas locales. Una solicitud adicional recibe ERROR(BUSY), no se ejecuta ni se
reintenta automáticamente. Cada nodo mantiene como máximo una orden CLI pendiente.
Mientras espera, sigue atendiendo mensajes de servicio. Evitar hilos por ahora.

Esto propone un orden total de accesos completados que respeta el orden de cada nodo:
la operación siguiente no se concede hasta terminar la anterior. Bajo una sesión sin
fallos, las lecturas deben observar la última escritura de ese orden. La coherencia
por página y este orden de ejecución son conceptos distintos; no basta con invalidar
copias para afirmar consistencia secuencial. La implementación tendrá que verificar
estas reglas; todavía no hay una garantía demostrada por código.

## Mensajes

Todos llevan tipo, origin_id del solicitante original, request_id, page_id, versión,
owner_id, flags y longitud. La identidad del emisor físico se conoce por la conexión;
no confundirla con origin_id cuando N0 reenvía una solicitud.

| Mensaje | Emisor → receptor; momento | Información específica | Respuesta esperada |
|---|---|---|---|
| HELLO | N1 → N0 al conectar | Identidad, tamaño/número de páginas, versión de protocolo | HELLO de N0 compatible o ERROR y cierre |
| READ_REQUEST | Solicitante → N0 al leer, incluso hit local | Estado y versión que declara la copia local | GRANT READ si válida; PAGE_DATA READ si falta; ERROR |
| WRITE_REQUEST | Solicitante → N0 antes de cualquier escritura | Estado/versión local; texto queda pendiente en CLI | GRANT WRITE o PAGE_DATA WRITE después de invalidaciones; ERROR |
| FETCH_PAGE | N0 → dueño si faltan bytes para atender un acceso | Página y versión confirmada esperada | PAGE_DATA FETCH_REPLY o ERROR |
| PAGE_DATA | Dueño → N0 como respuesta a FETCH; N0 → solicitante en un fallo | Página completa y versión; modo FETCH_REPLY, READ o WRITE | En FETCH: continuar transacción; en READ/WRITE: ACK de finalización |
| GRANT | N0 → solicitante cuando ya tiene bytes válidos | Modo READ/WRITE, versión base y propietario previsto | ACK READ_DONE o WRITE_DONE |
| INVALIDATE | N0 → cualquier otro poseedor válido antes de escribir | Página, versión anterior e identidad de transacción | ACK INVALIDATED después de invalidar |
| ACK | Receptor del evento → N0 | Fase INVALIDATED, READ_DONE o WRITE_DONE; versión aplicada | Sin ACK de ACK; avanzar o cerrar transacción |
| ERROR | Receptor que detecta error → solicitante/N0 | Código y breve descripción acotada | Ninguna; BUSY permite nueva solicitud manual |

GRANT y FETCH_PAGE hacen explícitos permiso y recuperación de datos. Sin ellos,
PAGE_DATA tendría significados implícitos difíciles de explicar. No hay mensajes para
consenso, descubrimiento, replicación del directorio ni recuperación de sesiones.

## Flujos completos A–E

### A. Lectura local válida

1. CLI valida el identificador y solicita READ a N0 (despacho local si es N0).
2. N0 reserva la operación y comprueba el bit válido y versión del solicitante.
3. Emite GRANT READ; no transfiere bytes de página.
4. Solicitante lee su arreglo, muestra el resultado y envía READ_DONE.
5. N0 libera la operación. Estado y versión no cambian.

Una copia local válida no implica acceso sin coordinación en este diseño.

### B. Lectura remota, estado INVALID

1. Se registra PAGE_FAULT simulado y se solicita READ a N0.
2. N0 toma los bytes del dueño: localmente si él los posee, o FETCH_PAGE/PAGE_DATA.
3. Envía PAGE_DATA READ al solicitante, que instala bytes y versión.
4. Si no es dueño queda READ_ONLY; el dueño conserva OWNER.
5. Tras READ_DONE, N0 agrega al lector a valid_mask y termina la operación.

Si N0 solo recibe bytes para reenviar, su buffer de transacción no es una copia local
válida: no agregar N0 al directorio salvo que sea el solicitante que instala la página.

### C. Escritura por el dueño actual

1. Se valida texto y límites antes de pedir WRITE.
2. N0 comprueba propietario y versión; si otro nodo tiene copia, envía INVALIDATE.
3. Espera todos los INVALIDATED. Si no hay lectores, omite esos mensajes.
4. Emite GRANT WRITE; solo ahora se reemplaza el contenido local de la página.
5. El dueño incrementa versión y envía WRITE_DONE; N0 confirma la versión y deja
   valid_mask con solo el dueño. No cambia propietario.

### D. Escritura por el otro nodo

1. Se pide WRITE; N0 reserva la transacción.
2. Si solicitante no tiene copia válida de la versión actual, se conserva una página
   completa del dueño en el buffer de transacción. Esto sucede **antes** de invalidarlo.
   Si el solicitante ya tiene READ_ONLY vigente, sus bytes son suficientes.
3. N0 invalida a todo poseedor distinto del solicitante, incluido el dueño anterior;
   espera INVALIDATED. El antiguo dueño no puede modificar ni servir nuevos accesos.
4. Envía PAGE_DATA WRITE si faltaba copia, o GRANT WRITE si ya era válida.
5. Solicitante instala si corresponde, pasa a OWNER pendiente y aplica la escritura.
6. WRITE_DONE confirma nueva versión; N0 publica nuevo dueño y máscara de un solo bit.
   Durante la transferencia, el directorio anterior está marcado por la transacción
   activa y no habilita otros accesos.

### E. Acceso después de invalidación

Los bytes residuales se ignoran. READ sigue B y recupera la última versión; WRITE sigue D.
`version` local obsoleta no puede convertir INVALID en un hit. Nunca usar datos viejos
para reconstruir una página que el propietario ya modificó.

## Formato y transporte futuros

Propuesta v1: cabecera de 40 bytes, diez enteros uint32 en orden de red:
magic, protocol_version, type, origin_id, request_id, page_id, owner_id, page_version,
flags, payload_len. No transmitir el enum C ni padding. Para mensajes sin página usar
UINT32_MAX en page_id; para dueño desconocido usar UINT32_MAX en owner_id.

PAGE_DATA tiene exactamente 4096 bytes. READ/WRITE_REQUEST llevan estado local en flags
y versión declarada; GRANT usa flags para modo. ACK usa flags para su fase. HELLO lleva
12 bytes (node_id, page_count, page_bytes); ERROR lleva código uint32 y texto UTF-8
acotado. Payload máximo: una página. Validar tamaños exactos por tipo antes de reservar
memoria o acceder a arrays. Los valores numéricos de flags y códigos se fijarán en el
issue de contrato antes de implementar la serialización.

Correlacionar (origin_id, request_id), tipo y fase esperada; rechazar ACK inesperados.
Cada nodo solo acepta los tipos permitidos por su rol y fase. No hay reintento automático
tras desconexión. Una trama puede llegar fragmentada o varias pueden llegar juntas:
acumular cabecera/cuerpo y mantener offset de envío, atendiendo EINTR y EAGAIN.
Referencias: [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html),
[recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html),
[send(2)](https://man7.org/linux/man-pages/man2/send.2.html).

## Errores y límites

Página/texto inválido: rechazo local antes de enviar. BUSY: no cambia memoria.
Versión o fase incoherente: error de protocolo, detener sesión. EOF, timeout monotónico
(propuesta inicial: 5 s por operación) o envío fallido durante una transacción: detener
ambos nodos al detectar cierre/plazo, sin seguir sirviendo datos ni asumir rollback.
No es posible recuperar de forma segura una propiedad a medio transferir con este MVP.
`exit` espera completar la orden pendiente; caída forzada aborta la sesión.
