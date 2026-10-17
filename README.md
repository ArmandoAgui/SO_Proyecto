# Mini-simulador DSM a nivel de página

Proyecto 10 — Sistemas Operativos. Equipo de cinco integrantes.

## Estado actual

**Fase 1 funcional:** dos procesos C independientes se conectan por una única conexión
TCP IPv4 sobre `127.0.0.1`, intercambian `HELLO 0` / `HELLO_ACK 1` y mantienen la
conexión hasta el cierre. `main`, `node` y `network` están implementados para esta fase;
`memory`, `protocol` y `cli` siguen como stubs.

Todavía no existen páginas, DSM real, READ_REQUEST, WRITE_REQUEST, PAGE_DATA,
INVALIDATE, consistencia ni transferencia de propiedad. El resto del diseño describe
trabajo futuro. Ver el [reporte de Fase 1](docs/planning/FASE1_SOCKETS.md).

## Alcance mínimo viable futuro

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

## Decisión de consistencia propuesta (futura)

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

## Compilar y ejecutar la Fase 1

Requisitos: Linux, Make y compilador compatible con C11 (GCC o Clang). El ejecutable
solo usa APIs Linux/POSIX, sin bibliotecas externas ni hilos. Python 3 es necesario
únicamente para las pruebas de integración.

```sh
make clean
make
./build/dsm-node --help
```

Abre dos terminales en la raíz del repositorio. Terminal 1:

```sh
./build/dsm-node --id 0 --port 5000 --peer-port 5001
```

Terminal 2:

```sh
./build/dsm-node --id 1 --port 5001 --peer-port 5000
```

Ambos escuchan. Solo N0 conecta y N1 acepta, evitando dos conexiones independientes.
Puedes iniciar cualquiera primero: N0 intenta hasta 30 veces, con pausas de 1 segundo
si el peer todavía no escucha; N1 espera hasta recibir conexión o una señal de cierre.
Los IDs válidos son 0 y 1; los puertos deben ser distintos y estar entre 1 y 65535.
Cada `--peer-port` debe coincidir con el `--port` del otro proceso.

Resultado esperado (algunas líneas):

```text
[NODE 0] Escuchando en 127.0.0.1:5000
[NODE 0] Conexión establecida con Nodo 1
[NODE 0] Mensaje enviado: HELLO 0
[NODE 0] Mensaje recibido: HELLO_ACK 1
[NODE 0] Intercambio HELLO completo. Enter o exit para cerrar.

[NODE 1] Escuchando en 127.0.0.1:5001
[NODE 1] Conexión recibida y aceptada desde 127.0.0.1
[NODE 1] Mensaje recibido: HELLO 0
[NODE 1] Mensaje enviado: HELLO_ACK 1
[NODE 1] Intercambio HELLO completo. Enter o exit para cerrar.
```

La conexión permanece abierta tras el intercambio. Pulsa Enter o escribe `exit`
seguido de Enter en cualquiera de las terminales: ese nodo cierra sus sockets y el
otro detecta EOF, informa `Peer desconectado` y también finaliza. EOF en stdin,
Ctrl+C y SIGTERM permiten cerrar; durante conexión/aceptación usa Ctrl+C.
Sin actividad durante el HELLO por 10 segundos, la sesión termina con error.
Un puerto ocupado o un mensaje inválido también genera un error comprensible y
un código de salida distinto de cero. No hay reconexión automática tras una sesión.

```sh
make test   # Pruebas C de transporte y 13 pruebas de integración Python
make help
make run ARGS='--id 0 --port 5000 --peer-port 5001'
make clean
```

Las pruebas usan 5000/5001 para verificar los comandos exactos; esos puertos deben
estar libres. Las demás pruebas eligen puertos temporales. `build/` está ignorado
por Git. Detalles de cobertura en [tests/README.md](tests/README.md).
