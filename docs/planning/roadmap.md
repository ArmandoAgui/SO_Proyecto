# Desarrollo incremental

Cada fase se integra a develop solo cuando cumple sus criterios. Los archivos C
mencionados son destinos previstos; hoy contienen únicamente stubs. Para cada fase,
las pruebas mínimas indicadas forman su puerta de aceptación.

| Fase | Objetivo y archivos | Funcionalidad | Aceptación y pruebas mínimas | Depende de |
|---|---|---|---|---|
| 0 — Preparación | README, CONTRIBUTING, Makefile, src/main.c y encabezados | Árbol, contratos iniciales, compilación de stubs | make desde limpio sin warnings; ejecutable explica que es esqueleto; todavía sin prueba DSM | Ninguna |
| 1 — Sockets | src/network/network.c, src/node/node.c, tests/unit/ y tests/integration/ | Conectar N0/N1, HELLO, codec y buffers incrementales; event loop atiende stdin/red | Mensaje idéntico recibido; probar cabecera/cuerpo fragmentados, dos tramas juntas, EOF y configuración distinta; sin congelar bucle | 0; contrato de mensajes revisado |
| 2 — Memoria | src/memory/memory.c, include/dsm/types.h | Arreglo contiguo, estados, versiones y directorio N0 | N0 OWNER/N1 INVALID; páginas 0 y 255 válidas; 256 rechazada; instalar/invalidar; sin leer INVALID | 0; puede avanzar en paralelo con 1 |
| 3 — Lectura local | memory.c, protocol.c, pruebas unitarias | API de lectura y autorización local inicial; orden completada | Página inicial devuelve ceros; INVALID devuelve falta de copia sin leer bytes; versión no cambia | 2 |
| 4 — Lectura remota | protocol.c, node.c, network.c, integración | READ_REQUEST, FETCH_PAGE, PAGE_DATA, GRANT READ y READ_DONE; directorio y operación global | N1 falla, instala y relee misma versión sin otro envío de página; también N0 lee desde dueño N1 en fixture controlado | 1, 3; entrada de órdenes de prueba |
| 5 — Propiedad exclusiva | protocol.c, memory.c | WRITE_REQUEST, concesión y WRITE_DONE, instantánea y cambio de dueño en caso sin lectores adicionales | Solo escritor autorizado modifica; otro dueño queda INVALID antes de escribir; versión aumenta una vez | 4; INVALIDATE/ACK mínimo incluido aquí |
| 6 — Invalidación completa | protocol.c, page-states.md, escenarios | Dueño que escribe con lectores; upgrade READ_ONLY; control de ACK y BUSY | No concesión antes de INVALIDATED; releer copia invalidada recupera valor nuevo; dos solicitudes concurrentes no dan doble concesión | 5 |
| 7 — CLI | cli.c, node.c, demo/ | Parser completo, help/status, límites, salida y trazas | Todos los comandos y errores de sintaxis; leer con el otro terminal inactivo funciona; stdin no bloquea servicios | 6; parser puede desarrollarse desde 0 |
| 8 — Errores | network.c, node.c, protocol.c | Timeout, EOF, SIGPIPE, tamaños/tipos inválidos y cierre seguro | Desconexión y trama incompleta terminan sin continuar con propiedad incierta; recursos liberados | 7; validaciones básicas existen desde fases anteriores |
| 9 — Integración | tests/integration/, docs/testing/ | Automatizar escenario completo y carreras controladas; sanitizers si disponibles | Todos los escenarios documentados pasan; guardar órdenes, resultado y commit; sin errores de memoria detectados | 8 |
| 10 — Entrega | demo/, docs/ieee/, presentation/, README | Demo repetible, evidencias, artículo IEEE y diapositivas | Otra persona repite demo; tablas basadas en mediciones; limitaciones y contribuciones reales; entrega main etiquetada | 9 |

La invalidación **no puede posponerse por completo** hasta fase 6: ceder propiedad en
fase 5 ya exige invalidar al dueño antiguo y esperar su ACK. Fase 6 generaliza a todas
las copias y a las escrituras del dueño con lectores. Ningún hito intermedio promete
una escritura segura sobre casos aún no implementados: se rechazan explícitamente.

Fase 3 usa harness unitario; fases 4–6 pueden usar órdenes de prueba mínimas sin
esperar a la CLI definitiva. No duplicar un segundo protocolo para pruebas.
El primer MVP funcional se declara después de fase 9; fase 10 lo convierte en entrega.
