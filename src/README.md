# Código de Fase 1

`main.c` valida `--id`, `--port`, `--peer-port` y ofrece `--help`. `node/node.c`
coordina arranque, conexión única N0 → N1, HELLO/HELLO_ACK, eventos y cierre.
`network/network.c` implementa sockets TCP IPv4, envío completo y líneas incrementales.

`memory`, `protocol` y `cli` siguen como stubs que retornan DSM_NOT_IMPLEMENTED.
No se reservan páginas ni se ejecutan operaciones DSM. El diseño futuro está en
`docs/architecture/`; la implementación actual se explica en
`docs/planning/FASE1_SOCKETS.md`.
