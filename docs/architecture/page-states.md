# Estados y transiciones implementados

| Estado | Significado |
|---|---|
| INVALID | No se permite leer, aunque existan bytes residuales. |
| READ_ONLY | Copia válida; no se permite escribir sin transacción exclusiva. |
| OWNER | Copia autoritativa; también necesita autorización del coordinador para modificar. |

| Operación/evento | Transición |
|---|---|
| Inicio | N0 OWNER; N1 INVALID. |
| read remoto | Se preserva una instantánea, se invalida al dueño anterior, se instala OWNER en solicitante. |
| read de READ_ONLY | Se invalida al otro poseedor y se promueve solicitante a OWNER. |
| read-copy remoto | Dueño conserva OWNER; solicitante instala READ_ONLY. |
| read-copy sobre copia válida | Conserva estado y versión; autorización sin transferencia de página. |
| write | Invalida a los otros, instala/promueve OWNER en solicitante y aumenta versión tras permiso. |
| INVALIDATE válido | OWNER o READ_ONLY pasa a INVALID antes del ACK. |
| EOF/timeout con operación | Abortan sesión; no se declara recuperación de estado. |

`read` deja una copia válida única; `read-copy` permite OWNER y READ_ONLY al mismo tiempo.
Esto cumple la migración exigida por el PDF y permite demostrar invalidación de lectores.

Invariantes en reposo:

1. Existe exactamente un OWNER y coincide con el directorio de N0.
2. Toda copia READ_ONLY tiene los mismos datos y versión que el dueño.
3. Las escrituras solo se ejecutan con concesión y sin otras copias válidas.
4. Ninguna lectura utiliza INVALID.
5. El directorio cambia al completar ACK_DONE, no al iniciar la transferencia.

Durante la operación puede haber cero OWNER. La instantánea evita perder bytes y el
bloqueo global impide usar ese estado intermedio para otra operación. Ante fallos se
aborta, sin intentar reconstruir una propiedad que podría haber quedado incierta.
