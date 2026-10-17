# Escenarios de aceptación actuales

Reiniciar la sesión entre casos independientes. Comprobar estados tras RESULT y,
si se consulta el directorio remoto, tras COMMIT. Leer no incrementa versión.

| Caso | Preparación/acción | Resultado |
|---|---|---|
| S01 | Conectar N0 y N1 | N0 OWNER y N1 INVALID en todas, versión 0. |
| S02 | N0 read 3 | Texto vacío, sin PAGE_DATA, dueño N0. |
| S03 | N0 write 3 hola; N1 read 3 | Fallo y migración: N0 INVALID, N1 OWNER, versión 1. |
| S04 | Repetir N1 read 3 | Sin nueva transferencia; conserva propiedad y versión. |
| S05 | Tras S03, N0 read-copy 3 | N0 READ_ONLY, N1 OWNER; mismos datos/versiones. |
| S06 | Tras S05, N1 write 3 nuevo | N1 pide invalidación; N0 INVALID antes de GRANT y escritura; versión 2. |
| S07 | N0 read 3 | Migra de vuelta y obtiene nuevo, N0 OWNER y N1 INVALID. |
| S08 | Nueva sesión: N1 write 255 remoto | Snapshot, invalidación N0 y transferencia, N1 OWNER versión 1. |
| S09 | N0 dueño con lector N1; escritura N0 y ACK retrasado | No escribe antes de ACK, solicitud concurrente recibe BUSY. |
| S10 | Rango 256, longitud 4096, ACK incorrecto, EOF parcial | Errores locales no modifican; fallos de sesión abortan. |

Los tests comprueban ambas direcciones, upgrade READ_ONLY y datos de múltiples páginas.
S09 usa un peer controlado y prueba C; no se depende de teclear a la vez.
La diferencia frente al plan original es explícita: read migra, read-copy replica.
