# Fuentes primarias verificadas

Consulta: 17 de octubre de 2026. Se revisaron las publicaciones originales y manuales;
no se usan artículos de terceros para justificar el formato TCP o la consistencia.
Las explicaciones de la implementación se basan en el código de este repositorio.

1. K. Li y P. Hudak, “Memory Coherence in Shared Virtual Memory Systems,”
   ACM Transactions on Computer Systems, vol. 7, n.º 4, pp. 321–359, noviembre 1989.
   DOI: [10.1145/75104.75105](https://doi.org/10.1145/75104.75105).
   [Texto original alojado en Berkeley](https://people.eecs.berkeley.edu/~prabal/resources/osprelim/LH89.pdf).
   Sustenta el antecedente de memoria virtual compartida por páginas y gestores
   centrales o distribuidos. Nuestro simulador usa accesos explícitos, no el sistema IVY.
2. L. Lamport, “How to Make a Multiprocessor Computer That Correctly Executes
   Multiprocess Programs,” IEEE Transactions on Computers, vol. C-28, n.º 9,
   pp. 690–691, septiembre 1979.
   [Texto original en Microsoft Research](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/12/How-to-Make-a-Multiprocessor-Computer-That-Correctly-Executes-Multiprocess-Programs.pdf).
   Fuente de la noción de consistencia secuencial; se distingue de la coherencia por página.
3. Linux man-pages, [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html).
   TCP es flujo de bytes; las fronteras lógicas deben implementarse en la aplicación.
4. Linux man-pages, [recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html).
   Recepción parcial, EOF, MSG_DONTWAIT y EAGAIN.
5. Linux man-pages, [send(2)](https://man7.org/linux/man-pages/man2/send.2.html).
   Envíos parciales y MSG_NOSIGNAL; base del tratamiento de EPIPE.
6. Linux man-pages, [poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html).
   Disponibilidad de descriptores para el bucle de eventos.
7. Guía de cátedra “C06_ProyectoCatedraSO.pdf”, proporcionada por el usuario:
   páginas 2, 12 y 13. Define el alcance de entrega y demo; no es una publicación
   bibliográfica sobre DSM ni se publica automáticamente con el código.

No se reproducen párrafos extensos ni figuras de estas fuentes. El artículo contiene
una síntesis propia breve y describe principalmente las decisiones y pruebas del simulador.
