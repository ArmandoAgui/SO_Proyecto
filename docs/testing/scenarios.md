# Escenarios de aceptación futuros

Reiniciar la sesión entre casos independientes. v indica la versión confirmada; leer
no la incrementa. Comprobar el estado en reposo y eventos de cada solicitud.

| Caso | Preparación y acción | Resultado esperado |
|---|---|---|
| S01 — Inicio | Conectar N0 y N1 compatibles | Todas páginas N0 OWNER/N1 INVALID; dueño 0 y versión 0 |
| S02 — Lectura local | N0 read 3 | Ceros/texto vacío; sin PAGE_DATA, sin fallo; v0 |
| S03 — Lectura remota | N0 write 3 hola; N1 read 3 | N1 fallo, recibe hola v1; N0 OWNER/N1 READ_ONLY |
| S04 — Hit de lectura | Después de S03, N1 read 3 | hola v1; control GRANT, sin nuevo PAGE_DATA ni fallo |
| S05 — Dueño con lector | Después de S03, N0 write 3 nuevo | N1 INVALID y ACK antes de escritura; N0 OWNER v2 |
| S06 — Relectura invalidada | Después de S05, N1 read 3 | Nuevo fallo y nuevo v2; jamás hola |
| S07 — Transferencia | Después de S06, N1 write 3 adios; N0 read 3 | N0 se invalida antes de escritura; dueño N1 v3; lectura N0 obtiene adios y queda READ_ONLY |
| S08 — Escritor sin copia | Sesión nueva: N1 write 4 remoto | Snapshot previa a invalidar N0; N1 OWNER v1, N0 INVALID; otros bytes según reemplazo documentado |
| S09 — Simultaneidad | Solicitar escrituras en ambos nodos antes de completar primera | Una activa y otra BUSY; reintento ordenado, sin dos concesiones activas; conservar versión más reciente |
| S10 — Errores y cierre | Rango 256, texto largo, ACK incorrecto, desconexión durante transferencia | Errores locales no mutan datos; fallo de protocolo/transporte aborta sesión, sin fingir rollback |

En S09 forzar el intercalado con el harness; no confiar en teclear al mismo tiempo.
Ejecutar también la dirección inversa de transferencia y varias páginas independientes.
Una lectura remota transfiere contenido, pero conserva al dueño: distinguir réplica de
lectura y migración de propiedad en el informe.
