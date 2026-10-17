# Encabezados

`dsm/node.h` define el contexto mínimo DSMNode y su ejecución; `dsm/network.h`
declara sockets, envío completo y buffer de líneas. `dsm/types.h` mantiene DSMStatus
con resultados de transporte y el estado DSM_NOT_IMPLEMENTED para los stubs.

`memory.h`, `protocol.h` y `cli.h` conservan sus declaraciones iniciales sin
implementar. Page, Message y el contexto DSM ampliado de
`docs/architecture/data-model.md` son propuestas futuras.
