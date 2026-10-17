# Objetivo

Implementar exclusivamente la primera simulación funcional: dos procesos C11 en
Linux conectados mediante TCP IPv4 sobre localhost, con intercambio textual
HELLO/HELLO_ACK y cierre limpio. Se conservan los módulos existentes; no se
implementa el simulador DSM completo.

# Archivos modificados

- `Makefile`: conserva compilación, dependencias y clean; agrega pruebas reales,
  argumentos para run y actualiza help.
- `README.md`: estado real, requisitos, comandos y resultados reproducibles.
- `src/main.c`: ayuda y validación de argumentos.
- `src/node/node.c`: ciclo de vida, conexión única, saludo, eventos, logs y cierre.
- `src/network/network.c`: servidor/cliente TCP, envío completo y framing incremental.
- `include/dsm/node.h`: contexto DSMNode y firma de ejecución con contexto.
- `include/dsm/network.h`: contratos del transporte y buffer de recepción.
- `include/dsm/types.h`: conserva DSM_NOT_IMPLEMENTED y añade resultados de transporte.
- `src/README.md` y `include/README.md`: descripción de interfaces implementadas.
- `docs/architecture/architecture.md`: arquitectura actual y separación del diseño futuro.
- `docs/architecture/data-model.md`: distingue el contexto mínimo del contexto DSM futuro.
- `docs/architecture/protocol.md`: distingue saludo textual del protocolo DSM futuro.
- `docs/architecture/cli.md`: aclara que las órdenes DSM siguen pendientes.
- `docs/testing/test-plan.md`: referencia la suite de Fase 1 y conserva el plan futuro.
- `tests/README.md`: ejecución y cobertura de las pruebas implementadas.

Archivos nuevos:

- `tests/unit/network_test.c`: pruebas C del transporte.
- `tests/integration/test_sockets.py`: pruebas con procesos TCP reales.
- `docs/planning/FASE1_SOCKETS.md`: este reporte.

No se modificaron las implementaciones ni encabezados de memory, protocol o cli.

# Arquitectura

Ambos nodos crean su socket servidor con SO_REUSEADDR, bind a 127.0.0.1 y listen.
Solo N0 inicia conexión; N1 acepta una vez. Esto evita conexiones duplicadas y
sustituye el sentido de conexión propuesto inicialmente en la arquitectura.
El puerto origen del cliente es efímero; el servidor de N0 no transporta el saludo.

```text
Proceso N0                              Proceso N1
--id 0                                  --id 1
--port 5000                             --port 5001
--peer-port 5001                         --peer-port 5000

server_fd: escucha 127.0.0.1:5000         server_fd: escucha 127.0.0.1:5001
cliente connect ----------------------> accept
peer_fd <======= una conexión TCP =====> peer_fd
          HELLO 0\n ------------------>
                    <------------------ HELLO_ACK 1\n
          conexión abierta; poll(red, stdin)
          Enter / exit / EOF / señal -> cierre -> EOF remoto
```

Cada DSMNode guarda ID, puerto local, puerto remoto, server_fd y peer_fd, inicialmente
-1. Ambos servidores permanecen abiertos hasta el cierre, pero no se aceptan peers
adicionales. El peer-port de N1 se almacena y valida como configuración; N1 no conecta.

Network no conoce estados DSM. Node coordina el saludo y la entrada mínima.
No hay hilos, bibliotecas externas, memoria distribuida ni llamadas a los stubs.

# Flujo de ejecución

1. `main` reconoce `--help` o exige los tres argumentos, sin duplicados.
2. Valida ID 0/1 y puertos decimales de 1 a 65535, distintos entre sí. Rechaza
   desconocidos, valores ausentes, signos, texto y desbordamiento numérico.
3. `node` instala manejadores SIGINT/SIGTERM y registra el inicio.
4. Crea servidor TCP IPv4 en loopback y registra el puerto de escucha.
5. N0 intenta conectar a peer-port hasta 30 veces. Si recibe ECONNREFUSED espera
   1 segundo entre intentos (29 pausas como máximo); otros errores abortan.
   N1 espera una conexión; puede iniciarse primero y cancelarse con Ctrl+C.
6. N0 envía `HELLO 0\n`; N1 recibe y valida la línea completa.
7. N1 responde `HELLO_ACK 1\n`; N0 recibe y valida la respuesta.
8. `poll()` mantiene la sesión abierta, atiende red y stdin después del saludo.
   Los fragmentos permanecen en un buffer; no se espera bloqueando el resto de la línea.
9. Enter, `exit`, EOF o señal cierran localmente; el peer detecta EOF y también sale.
   Un cierre previo al saludo completo o una línea incompleta son errores.
10. Todos los caminos de sesión cierran peer_fd y server_fd; los fallos parciales
    al crear/conectar sockets liberan sus descriptores antes de retornar.

# Funciones nuevas

Interfaces públicas (se conserva el prefijo `dsm_` existente):

| Función | Responsabilidad |
|---|---|
| `dsm_node_run(DSMNode *)` | Completa el antiguo stub; administra la sesión y recursos. |
| `dsm_network_create_server` | socket, SO_REUSEADDR, bind y listen; retorna fd o -1. |
| `dsm_network_accept_peer` | Acepta una conexión y retorna su descriptor. |
| `dsm_network_connect_peer` | Conecta a 127.0.0.1:peer-port; libera fd si falla. |
| `dsm_network_send` | Repite send hasta enviar todos los bytes; maneja EINTR y MSG_NOSIGNAL. |
| `dsm_network_receive` | Acumula una línea con MSG_DONTWAIT; retorna OK, AGAIN, CLOSED o ERROR. |
| `dsm_network_close` | Cierra un descriptor abierto y lo deja en -1. |

Se sustituye `dsm_network_init(void)`, un stub sin consumidores, por estas funciones
concretas. `DSM_NOT_IMPLEMENTED` sigue vigente en los tres módulos pendientes.

Funciones internas: `print_help` muestra uso; `parse_number` valida números;
`local_address` construye la dirección IPv4; `request_stop` solo modifica la bandera
de señal; `node_error` contextualiza errno; `send_hello` registra y envía el saludo;
`establish_connection` implementa roles y reintentos; `run_session` recibe, valida y
atiende stdin sin implementar la CLI DSM.

La recepción lee byte a byte para explicar fácilmente la delimitación por `\n`:
no pierde la siguiente línea si TCP entrega dos juntas. El límite es 127 bytes de
contenido más terminador NUL; rechaza contenido NUL y longitudes excesivas.
EOF parcial produce EPROTO. Esta solución favorece claridad sobre rendimiento.

# Cómo compilar

Desde la raíz del repositorio, con Linux, Make y compilador C11:

```sh
make clean
make
./build/dsm-node --help
make help
```

Se genera `build/dsm-node` con `-std=c11 -Wall -Wextra -Wpedantic -g` por defecto.

# Cómo ejecutar

Terminal 1:

```sh
./build/dsm-node --id 0 --port 5000 --peer-port 5001
```

Terminal 2:

```sh
./build/dsm-node --id 1 --port 5001 --peer-port 5000
```

También se admite iniciar Terminal 2 antes de Terminal 1. En ambos casos, cada puerto
remoto debe coincidir con el local del otro nodo. Tras el saludo, pulsa Enter o
escribe `exit` seguido de Enter en cualquiera de las terminales. Ctrl+C funciona
también durante el arranque. Si se redirige stdin desde /dev/null, EOF cierra la
sesión al completar el saludo; para una demo persistente usa terminales abiertas.

# Resultado esperado

Ejemplo cuando N0 inicia primero (el número de reintentos depende de cuándo inicia N1):

```text
[NODE 0] Iniciando nodo
[NODE 0] Escuchando en 127.0.0.1:5000
[NODE 0] Intentando conectar con 127.0.0.1:5001 (1/30)
[NODE 0] Peer todavía no disponible. Reintentando en 1 segundo...
[NODE 0] Intentando conectar con 127.0.0.1:5001 (2/30)
[NODE 0] Conexión establecida con Nodo 1
[NODE 0] Mensaje enviado: HELLO 0
[NODE 0] Mensaje recibido: HELLO_ACK 1
[NODE 0] Intercambio HELLO completo. Enter o exit para cerrar.
[NODE 0] Cierre solicitado por el usuario
[NODE 0] Conexión cerrada
[NODE 0] Socket servidor cerrado. Nodo finalizado
```

```text
[NODE 1] Iniciando nodo
[NODE 1] Escuchando en 127.0.0.1:5001
[NODE 1] Esperando conexión de Nodo 0
[NODE 1] Conexión recibida y aceptada desde 127.0.0.1
[NODE 1] Mensaje recibido: HELLO 0
[NODE 1] Mensaje enviado: HELLO_ACK 1
[NODE 1] Intercambio HELLO completo. Enter o exit para cerrar.
[NODE 1] Peer desconectado; conexión cerrada por el peer
[NODE 1] Conexión cerrada
[NODE 1] Socket servidor cerrado. Nodo finalizado
```

Los errores incluyen ID, operación y strerror(errno). Un fallo de servidor identifica
la etapa agrupada socket/SO_REUSEADDR/bind/listen y la causa del sistema. El proceso
retorna 0 ante cierre normal y distinto de cero ante error.

# Pruebas realizadas

Entorno de ejecución: Linux 7.1.5-76070105-generic, GCC 13.3.0; flags por defecto
indicados arriba. Fecha local de verificación: 2026-10-17.

```sh
make clean
make
make test
make help
./build/dsm-node --help
```

- Compilación completa: PASS, sin errores ni warnings.
- Pruebas C: PASS para fragmentación, dos líneas en una entrega, EOF normal/parcial,
  exceso de longitud, cierre idempotente, EPIPE sin SIGPIPE y envío íntegro de 64 KiB
  con buffer pequeño y un segundo proceso receptor.
- Integración: PASS, 13 pruebas en 40.589 segundos, incluyendo 13 variantes de
  argumentos inválidos. Comprueban
  ayuda, ambos órdenes de inicio, reintento, comandos exactos con 5000/5001,
  HELLO/HELLO_ACK en ambos sentidos y procesos vivos tras el saludo; Enter, exit,
  EOF, Ctrl+C, SIGTERM durante accept/reintento, puerto ocupado, HELLO fragmentado,
  EOF parcial, identidad inválida, timeout de HELLO y agotamiento de 30 intentos.
- No se inyectan fallos artificiales de listen/accept ni EINTR en cada syscall;
  existen comprobaciones en código. La prueba de envío verifica integridad, sin
  afirmar que el kernel devolvió necesariamente un envío parcial.

Los detalles de ejecución y requisitos de pruebas están en `tests/README.md`.

# Limitaciones actuales

Aún NO existen:

- Páginas.
- DSM real.
- READ_REQUEST.
- WRITE_REQUEST.
- PAGE_DATA.
- INVALIDATE.
- Consistencia.
- Transferencia de propiedad.

Solo IDs 0 y 1, una sesión sobre loopback y mensajes HELLO fijos. No hay envío
interactivo de texto arbitrario, autenticación, reconexión ni recuperación de caídas.
La espera de conexión de N1 es indefinida hasta conectar o recibir una señal.
El HELLO tiene timeout de 10 segundos sin actividad; el envío de los mensajes pequeños
es bloqueante. El transporte de páginas futuro requerirá otro framing y una cola de
envío apropiada, sin asumir que esta fase implementa ese protocolo.

# Próximo paso recomendado

Implementar el modelo local de memoria y páginas en una fase posterior, con límites,
metadatos y pruebas locales, antes de añadir transferencia o consistencia distribuida.
Esta tarea no avanza a esa fase.
