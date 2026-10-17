# Página lógica y fallo simulado

Una página lógica es un bloque de 4096 bytes del arreglo local del simulador. Cada
nodo reserva 256 bloques, pero solo puede leer aquellos cuyo estado no sea INVALID.
La reserva física de bytes y su validez lógica son conceptos distintos.

El acceso se hace mediante comandos; el chequeo de metadatos detecta la falta de copia
válida y registra un fallo simulado. No se interceptan instrucciones de memoria ni
SIGSEGV, ni se usa mmap/mprotect o swap. El tamaño de 4096 bytes es un parámetro
educativo común a ambos procesos, no una medición del tamaño de página de Linux.

El byte inicial de la página p está en p*4096; p se valida antes de calcular el acceso.
Una transferencia instala los 4096 bytes, no solo el texto visible. Una invalidación
puede dejar bytes viejos, que no vuelven a ser válidos sin autorización e instalación.
