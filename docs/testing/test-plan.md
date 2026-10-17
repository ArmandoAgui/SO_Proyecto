# Plan y cobertura de pruebas

La suite actual y resultados están en tests/README.md y RESULTADOS.md. Se ejecutan
pruebas C locales, integración TCP, oráculo de datos, carreras controladas y sanitizers.

Puertas actuales: build sin warnings; no leer INVALID; no escribir sin exclusividad;
no conceder antes del ACK; migrar en read; copiar sin migrar en read-copy; rechazar
tramas/campos incorrectos; abortar sin bloquear ante EOF/timeout; cerrar recursos.

Mejoras futuras fuera de la entrega mínima: exploración exhaustiva de interleavings,
fuzzing prolongado de cabeceras, fallos de asignación, inyección de errno en cada syscall,
pruebas sobre LAN y medición de latencia sin el polling del harness. No se presentan
estas ampliaciones como pruebas ya realizadas.
