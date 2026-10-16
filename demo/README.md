# Guion preliminar de demo

Todavía no ejecutable: faltan las funciones DSM. Usar dos terminales y rotular N0/N1.
Al llegar al MVP ejecutar la secuencia de README.md (página 3), consultando status
tras cada operación terminada. Resaltar PAGE_FAULT, transferencia, ACK de invalidación,
concesión de escritura y cambio de dueño. Terminar con lectura en el antiguo dueño.

Guardar versiones y logs por nodo junto con commit/configuración en docs/testing/.
Medir número de transferencias, fallos e invalidaciones y duración con reloj monotónico;
separar medición de lectura local/remota. No afirmar mejoras sin comparar ejecuciones.
