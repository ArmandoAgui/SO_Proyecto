# Resultados verificados

Fecha: 17 de octubre de 2026 (America/El_Salvador). Pop!_OS 24.04 LTS,
Linux 7.1.5-76070105-generic, GCC 13.3.0, C11, -Wall -Wextra -Wpedantic -g.

## Ejecuciones

| Verificación | Resultado |
|---|---|
| make clean; make | PASS, sin errores ni warnings. |
| make test: tres programas C | PASS. |
| make test: integración | PASS, 22 casos en 48.166 s. |
| make sanitize | PASS, tres programas C y nueve casos DSM; sin hallazgos ASan/UBSan. |
| make demo | PASS; ambas direcciones de propiedad y datos actualizados. |
| make docs | PASS; artículo IEEE de 3 páginas y presentación de 12 diapositivas, maquetación revisada. |
| make package | PASS; integridad ZIP y compilación sin warnings tras extraer a una carpeta nueva. |

La salida completa de la suite normal está en evidence/test-results.txt. Los errores
EPROTO que aparecen en la prueba C del protocolo son **inyecciones deliberadas** de
ACK incorrectos: se comprueba que sean rechazados antes de continuar con el ACK válido.

## Qué se comprobó

- Inicialización, estados, primera/última página y acceso fuera de rango.
- Lectura INVALID y escritura READ_ONLY rechazadas por memory.
- Versión, desbordamiento y reemplazo completo con ceros del resto de la página.
- Parser: negativos, textos vacíos, tokens extra, número enorme, 4095 bytes aceptados,
  4096 rechazados y recuperación después de una línea demasiado larga.
- TCP real en dos procesos, ambos órdenes de arranque, reintento, puertos 5000/5001,
  puerto ocupado y agotamiento de 30 intentos.
- Cabeceras/cuerpos fragmentados, mensajes juntos, tamaños/tipos/páginas incorrectos,
  EOF normal/parcial, SIGPIPE suprimido y cola con EAGAIN sin perder bytes.
- Migración N0 → N1 y N1 → N0, copia READ_ONLY, escritura del dueño con lector,
  upgrade de copia y escritor sin datos locales.
- Confirmación remota retrasada: ni GRANT_WRITE ni escritura antes de ACK; solicitud
  concurrente rechazada con BUSY. Un ACK con página/versión falsa no desbloquea.
- 48 operaciones pseudoaleatorias secuenciales, semilla 10, contrastadas con un
  modelo de valores esperados. No es una prueba aleatoria sin oráculo.
- Timeouts absolutos, peer que muere durante petición y salida Enter/exit/EOF/señales.

## Evidencia de la demo

Los archivos demo-node0.txt y demo-node1.txt son trazas reales. demo-summary.json
registra comandos, entorno, commit base, árbol modificado y SHA-256 de fuentes/
encabezados/Makefile. El commit base no se presenta como si contuviera los cambios
sin confirmar. No se modifica Git para fabricar una revisión de entrega.

Al finalizar los cinco accesos de la demo:

| Métrica | N0 | N1 |
|---|---|---|
| Fallos simulados | 2 | 1 |
| Tramas PAGE_DATA enviadas | 1 | 2 |
| Tramas PAGE_DATA recibidas | 2 | 1 |
| Invalidaciones locales aplicadas | 2 | 1 |
| Commits del coordinador | 5 | No aplica |

Son 3 transferencias completas de 4096 bytes: 12288 bytes de payload de páginas.
La segunda escritura conserva el dueño N1 y no retransmite la página: invalida N0
antes de modificar. La versión final de página 3 es 2 y el dueño final es N0.
Los contadores de envío cuentan mensajes encolados; en esta demo se comprueba su
recepción y cierre correcto. No deben usarse como prueba de recepción tras un fallo.

No se ha medido latencia pura ni rendimiento sobre una LAN. La duración de la suite
incluye esperas intencionales de 10 s y 29 s y planificación del harness. No equivale
a tiempo de acceso DSM. Tampoco se afirma cobertura de todos los interleavings ni
inyección de cada posible errno del kernel.

## Repetir

```sh
make test
make sanitize
make demo
```

make demo sobrescribe únicamente sus archivos de evidencia, no la salida histórica de
make test. Para actualizar esa salida, ejecutar explícitamente make test y redirigirla.
