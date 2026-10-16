# Mini-simulador DSM a nivel de página

Proyecto 10 — Sistemas Operativos. Equipo de cinco integrantes.

## Estado actual

Arquitectura, planificación y esqueleto C compilable. **Todavía no existe un simulador
DSM funcional**, ni se ha validado el protocolo con código. Los tipos y mensajes del
diseño son propuestas para implementar por fases. Los cinco módulos solo tienen stubs.

## Alcance mínimo viable

- C11 sobre Linux; dos procesos independientes (nodos 0 y 1).
- Una conexión TCP IPv4 persistente, inicialmente sobre `127.0.0.1`.
- Cada proceso reserva un arreglo contiguo de 1 MiB: 256 páginas lógicas de 4096 bytes.
  Son parámetros de diseño comunes a ambos nodos, no el tamaño de página exigido por Linux.
- Inicio: nodo 0 tiene todas las páginas inicializadas en cero y es su propietario;
  nodo 1 tiene todas las páginas INVALID. No se puede acceder a esos bytes inválidos.
- Acceso explícito mediante CLI y funciones DSM. El fallo de página se **simula** al
  detectar INVALID; no se interceptan accesos de C, SIGSEGV ni fallos del kernel.
- Lecturas remotas transfieren páginas completas y dejan copias de lectura.
- Escrituras obtienen exclusividad, invalidan otras copias y pueden transferir propiedad.
- `status` y registros muestran mensajes, fallos, estados, propietario y versiones.
- Coordinación central en el nodo 0; propiedad de datos móvil. Una operación de memoria
  global en curso, con órdenes concurrentes respondidas con BUSY para reintento manual.

Se elige TCP frente a UDP para no implementar retransmisión de paquetes. TCP es un
flujo sin fronteras de mensajes: el transporte debe delimitar y acumular tramas.
Fuente: [Linux tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html).

## Decisión de consistencia

Todas las lecturas y escrituras, incluso sobre copias locales, piden autorización al
coordinador. Una lectura local evita transferir datos, pero el nodo 1 todavía intercambia
mensajes de control. Este costo permite ordenar globalmente las operaciones sin hilos,
locks distribuidos ni un directorio replicado. El bucle de eventos permanece atendiendo
la red mientras una orden de CLI espera su respuesta.

OWNER significa dueño de la copia autorizada; **no es permiso permanente para escribir**:
si hay otro lector debe invalidarse y confirmar antes de conceder la escritura.
Ver [estados](docs/architecture/page-states.md) y [protocolo](docs/architecture/protocol.md).

## Cuándo existe el primer MVP

Con ambos procesos conectados y sin fallos de red, debe poder repetirse:

1. N0 escribe `hola` en página 3; N0=OWNER, N1=INVALID.
2. N1 lee página 3: fallo simulado, transferencia completa, obtiene `hola`;
   N0=OWNER y N1=READ_ONLY, misma versión.
3. N0 escribe `nuevo`: N1 confirma invalidación **antes** de la escritura.
4. N1 vuelve a leer: nuevo fallo y obtiene `nuevo`.
5. N1 escribe `adios`: N0 queda INVALID y N1 pasa a OWNER.
6. N0 lee y obtiene `adios`; N0=READ_ONLY y N1=OWNER.
7. `status` en ambas terminales y las trazas concuerdan con el directorio de N0.
8. Pasan los escenarios de coherencia, límites, cierre y órdenes simultáneas descritos
   en el [plan de pruebas](docs/testing/test-plan.md), sin doble propietario estable.

Una conexión TCP o una lectura remota sola es un hito parcial; no es el MVP completo.

## Ruta de lectura y trabajo

- [Arquitectura y módulos](docs/architecture/architecture.md)
- [Estructuras de datos propuestas](docs/architecture/data-model.md)
- [Protocolo y flujos A–E](docs/architecture/protocol.md)
- [CLI](docs/architecture/cli.md)
- [Fases 0–10](docs/planning/roadmap.md)
- [Reparto entre cinco integrantes](docs/planning/team.md)
- [Issues preparados para GitHub](docs/planning/issues.md)
- [Primeras tareas y árbol del repositorio](docs/planning/start-today.md)
- [Git y contribuciones](CONTRIBUTING.md)
- [Esquema IEEE](docs/ieee/outline.md)

## Fuera de alcance inicial

Más de dos nodos, tolerancia a caídas, reconexión con recuperación, swapping, reemplazo
LRU, mmap/mprotect, multiprocesamiento interno, interfaz gráfica, descubrimiento,
consistencia relajada y optimizaciones de rendimiento. No simular latencia artificial
antes de tener mediciones reales. El nodo 0 es un punto único de fallo aceptado.

## Compilar y verificar el esqueleto

```sh
make
./build/dsm-node --help
make run
make clean
```

Requisitos: Linux, Make y compilador compatible con C11. `make` compila main y los cinco
módulos. `make run` imprime que no hay simulación; no abre sockets ni atiende read/write.
No existe todavía un target de pruebas DSM. `build/` contiene artefactos ignorados por Git.
