# Arquitectura implementada

Dos procesos C11 en Linux, mismos binarios con ID 0/1. Ambos escuchan en loopback;
solo N0 conecta al puerto de N1, con 30 intentos y pausas de 1 s ante ECONNREFUSED.
Una conexión TCP bidireccional sirve para control y datos. Se conserva el saludo textual
de Fase 1 y después se intercambia HELLO_CONFIG binario con identidad y geometría.

```text
Terminal N0 -> CLI -> protocolo/coordinador -> memoria local N0
                          | directorio
                          | transacción global
                          v
                     cola TCP N0 <======> cola TCP N1
                                              |
                                         protocolo N1
                                         /          \
                                    CLI N1       memoria N1
```

| Módulo | Responsabilidad implementada |
|---|---|
| main | Validación de configuración y ayuda de arranque. |
| node | Recursos, señales, conexión, eventos, entrada incremental, plazos y cierre. |
| network | Sockets, saludo textual, codec binario, recepción incremental y cola parcial. |
| memory | Arreglo contiguo y operaciones locales comprobadas, sin sockets. |
| protocol | Directorio autoritativo N0, transacción, permisos, versiones, BUSY y trazas. |
| cli | Parser sin ejecución de shell y salida de datos con escapes. |

Cada proceso reserva 1 MiB: 256 páginas de 4096 bytes. No se realiza mapeo compartido
entre procesos. El contexto de configuración DSMNode sigue separado del contexto de
sesión, que agrupa protocolo y buffers. El directorio se reserva como parte del contexto
en ambos nodos para simplificar C, pero **solo es autoritativo y utilizado en N0**.

## Coordinación y ejecución

Un único hilo por proceso. poll atiende socket y stdin (después de negociar geometría).
Se reciben hasta 32 mensajes por iteración para no monopolizar el bucle. Tanto recibir
tramas como vaciar la cola usan MSG_DONTWAIT, conservando los offsets ante EAGAIN.
La entrada de terminal se acumula hasta newline; no se llama fgets bloqueando la red.

Todas las lecturas/escrituras, incluso los hits locales, pasan por N0. Una transacción
está IDLE, FETCHING, INVALIDATING o WAIT_DONE. La instantánea de 4096 bytes conserva
los datos antes de invalidar al dueño. No se acepta otra operación global hasta ACK_DONE;
se responde BUSY sin reintento automático. Un nodo solo tiene una petición local pendiente.

La guía del docente exige migración en lectura: `read` cede la propiedad e invalida a
los demás. `read-copy` deja una copia READ_ONLY y mantiene al dueño. Es una corrección
respecto del diseño inicial que replicaba todas las lecturas. No es una reescritura de
la arquitectura: se mantienen módulos, coordenador N0 y una conexión.

## Escrituras e invalidaciones

Antes de conceder WRITE se invalidan todos los holders distintos del solicitante.
Si el holder es N1, N0 manda INVALIDATE y espera ACK_INVALIDATED. Si es N0, aplica
la invalidación local antes de GRANT. Cuando N1 ya es dueño y escribe, manda INVALIDATE
como solicitud de exclusividad; N0 la procesa como transacción de escritura e invalida
su copia, si existe. Así la segunda parte de la demo muestra N1 → N0 INVALIDATE real.
N1 no modifica nada por enviar esa petición: espera la concesión.

## Fallos y cierre

Timeout absoluto de 10 s desde que empieza el saludo hasta recibir HELLO_CONFIG.
Plazo de 5 s para petición local, transacción global, trama incompleta y cola sin vaciar;
los plazos no se renuevan por recibir unos cuantos bytes. La precisión depende del
poll de 100 ms y de la planificación de Linux.

exit, Enter y EOF esperan a la operación en curso y vacían la cola. SIGINT/SIGTERM
solicitan aborto. EOF de red en reposo finaliza normalmente; durante transacción o
trama incompleta es error. No hay rollback ni recuperación: ambos reinician desde cero.
Cada socket fallido al crear/conectar se cierra; los recursos de sesión se liberan
al salir. MSG_NOSIGNAL impide que un EPIPE termine el proceso sin tratamiento.

## Directorios

src e include/dsm contienen código; tests, sus comprobaciones; tools, demo/empaquetado;
docs/architecture, los contratos actuales; docs/testing/evidence, trazas reales;
docs/ieee y presentation, documentos editables; build/entrega, PDFs/ZIP generados.
FASE1_SOCKETS.md y la planificación previa conservan el historial, no el estado actual.
