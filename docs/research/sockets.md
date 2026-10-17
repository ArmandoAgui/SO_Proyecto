# Sockets y transporte del simulador

Se utiliza TCP IPv4 en loopback con una conexión persistente. El transporte conserva
el saludo textual y después usa tramas binarias de cabecera fija. Los buffers de
entrada y salida conservan el progreso de operaciones parciales; MSG_DONTWAIT evita
bloquear el bucle cuando faltan bytes o espacio de envío. Se atienden EINTR, EAGAIN,
EOF y EPIPE; MSG_NOSIGNAL impide SIGPIPE. Los plazos y la política de sesión pertenecen
al módulo node, no a memory.

El contrato exacto está en ../architecture/protocol.md y los tests del codec en
../../tests/unit/network_test.c. Las referencias primarias verificadas figuran en
FUENTES.md: Linux tcp(7), recv(2), send(2) y poll(2).
