# Backlog inicial para GitHub

> Plan original conservado como historial. Para el estado actual, consulte
> `IMPLEMENTACION_DSM.md`, `ENTREGA_PROYECTO10.md` y el README de la raíz.

Estos son borradores locales; **no se han creado issues en GitHub**. I1–I5 se definen
en team.md. Numeración G01–G15 interna, independiente de los números remotos futuros.

| ID y título | Descripción breve | Responsable | Dependencias | Criterio de aceptación |
|---|---|---|---|---|
| G01 — Estructura compilable y guía de colaboración | Directorios, Makefile y stubs | I1 | Ninguna | make compila todos los módulos, ejecutable declara esqueleto; revisión de árbol y documentación |
| G02 — Cerrar contrato de estados y mensajes v1 | Revisar roles, flags, códigos, framing y flujos | I4 + revisión I2/I3 | G01 | Cada mensaje tiene formato exacto y transición; resolver OWNER con lectores y ACK de transferencia |
| G03 — Conexión TCP y HELLO | Arranque N0/N1 y configuración compatible | I2 | G02 | Dos procesos conectan; identidad o tamaño distintos se rechazan |
| G04 — Codec y transporte incremental | Cabecera/cuerpo, validación y colas no bloqueantes | I2 | G03 | Fragmentación y mensajes unidos funcionan; EOF/tamaños erróneos detectados |
| G05 — Memoria local y metadatos | Arreglo continuo, inicialización, instalar/invalidar y límites | I3 | G02 | Páginas 0/255 operables, 256 rechazada, INVALID no legible |
| G06 — Bucle de eventos y ciclo de vida | Atender stdin/socket, buffers y cierre | I1 | G04 | Terminal inactiva no bloquea mensajes; no recv_all bloqueante en bucle |
| G07 — API de lectura local | Operación autorizada y acceso a bytes válidos | I3 | G05 | Devuelve datos/versiones correctos; INVALID reporta falta sin leer |
| G08 — Lectura remota y directorio | READ_REQUEST/FETCH/PAGE_DATA/GRANT/ACK y operación global | I4 | G06, G07 | Primera lectura N1 transfiere, siguiente no; directorio coincide al terminar |
| G09 — Escritura y transferencia mínima | Concesión exclusiva, invalidar dueño anterior y WRITE_DONE | I4 + I3 | G08 | Escritor remoto pasa a OWNER; anterior INVALID antes de modificar; versión +1 |
| G10 — Invalidación de lectores y concurrencia | Escritura del dueño con copia remota, upgrade y BUSY | I4 | G09 | Espera ACK; nunca concede dos escrituras; releer recupera último valor |
| G11 — CLI y trazas | Parser, help/read/write/status/exit y eventos | I5 | G02 para parser; G10 para integración | Límites validados; datos con espacios; estados y fases visibles |
| G12 — Errores y finalización segura | Timeout, EOF, SIGPIPE, fase errónea y recursos | I1 + I2 | G10, G11 | Una sesión incierta aborta; no se anuncia éxito de operación incompleta |
| G13 — Suite de integración | Automatizar recorridos, simultaneidad y evidencia | I5 + todos | G12 | Escenarios S01–S10 reproducibles con salida y commit, fallos provocan resultado no exitoso |
| G14 — Demo y mediciones | Guion y datos de una ejecución identificada | I5 + I3 | G13 | Otra persona reproduce resultados; conservar tiempos y contadores sin inventar rendimiento |
| G15 — Artículo IEEE y presentación | Convertir evidencias en informe y exposición | Todos, coordina I1 | G14 para resultados; esquema desde G01 | Secciones trazables a diseño/código/pruebas; cinco contribuciones técnicas explicables |

Orden de apertura recomendado: G01, G02, G03, G05, G11 (solo parser y especificación).
Abrir los restantes al estabilizar contratos. Etiquetas sugeridas: `network`, `memory`,
`protocol`, `cli`, `testing`, `docs`; usar milestones por fase. No considerar terminado
un issue de funcionalidad por haber creado solo su archivo stub.
