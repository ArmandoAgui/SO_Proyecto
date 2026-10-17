# Coherencia y consistencia

Coherencia: las copias válidas de una página no divergen en reposo; modificar requiere
exclusividad e invalidar las demás. Consistencia secuencial: debe poder explicarse la
ejecución mediante un orden de accesos compatible con el orden de cada participante.
La referencia original de Lamport está en FUENTES.md.

El simulador ordena todos los accesos en N0. No admite uno nuevo hasta ACK_DONE del
anterior y los conflictos reciben BUSY. Ni siquiera un OWNER modifica por fuera de
esa autorización. Las versiones aumentan al escribir, no al copiar o migrar.

Bajo una sesión sin fallos, el orden de commits permite justificar los valores leídos.
La suite comprueba ejemplos y carreras controladas; no es una prueba formal de todos
los interleavings. No se promete consistencia recuperable tras desconexión: se aborta.
