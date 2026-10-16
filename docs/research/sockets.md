# Sockets — fuentes y decisión inicial

Recomendación: TCP IPv4 sobre loopback con una conexión persistente. TCP entrega un flujo
ordenado sin preservar límites de mensajes; diseñar framing explícito y buffers parciales.
Fuentes primarias: [Linux tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html),
[recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html) y
[send(2)](https://man7.org/linux/man-pages/man2/send.2.html).

Investigar para la fase 1: EINTR, EAGAIN, EOF y SIGPIPE. No asumir que una llamada a
send o recv procesa la trama completa. La implementación final atenderá la red sin
bloquear el bucle mientras espera datos adicionales.
