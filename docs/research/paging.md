# Paginación — nota inicial

Página lógica del simulador: bloque de 4096 bytes del arreglo local. Un acceso INVALID
produce un fallo simulado y recuperación mediante protocolo. No es una falta de página
manejada por Linux. Decisión educativa: 256 bloques, sin reemplazo, disco ni swap.
Pendiente: contrastar páginas virtuales reales y esta abstracción con las fuentes del curso.
