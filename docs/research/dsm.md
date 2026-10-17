# DSM y su relación con el simulador

DSM ofrece una abstracción de memoria lógica compartida sobre memorias separadas.
Li y Hudak estudiaron gestores centrales y distribuidos para mantener coherencia por
páginas. Aquí se utiliza esa idea de unidades de transferencia y propiedad, con un
coordinador didáctico en N0. Ver la referencia primaria en FUENTES.md.

Nuestro programa no implementa memoria virtual transparente: cada comando pide
explícitamente una página. Cuando está INVALID se registra PAGE_FAULT simulated=1.
La transferencia sí ocurre por sockets reales entre arreglos de procesos diferentes.
read migra la propiedad; read-copy conserva una copia de lectura. El tamaño de página
es una decisión del modelo, no una consulta de páginas físicas de Linux.
