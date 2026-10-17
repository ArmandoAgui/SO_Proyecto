# CLI implementada

Arranque: `--id 0|1 --port 1..65535 --peer-port 1..65535`; puertos distintos,
argumentos obligatorios, sin duplicados. `--help` muestra ejemplos y termina.

| Comando | Semántica |
|---|---|
| read p | Lectura con propiedad exclusiva al terminar; migra si no es dueño. |
| read-copy p | Lectura que puede conservar copias READ_ONLY. |
| write p texto | Reemplaza una página después de obtener exclusividad. |
| status [p] | Estado local, directorio N0, fase, solicitud y contadores. |
| help | Muestra la sintaxis. |
| exit o línea vacía | Espera operación pendiente y vacía cola antes de cerrar. |

p está en 0..255. El texto tiene 1..4095 bytes; el resto de la página se rellena con
ceros, incluido NUL. Se eliminan los espacios iniciales antes del texto y se conservan
los posteriores; no existe sintaxis de comillas ni expansión de shell.

El parser valida decimal, rango, tokens sobrantes y desbordamiento. Una línea con NUL
o demasiado larga se descarta hasta newline; la siguiente línea se procesa normalmente.
Los datos de página se imprimen con límite y escapes para comillas, backslash y bytes
no ASCII imprimibles. No se ejecuta texto suministrado por el usuario.

`status` sin página muestra conteos globales y las páginas modificadas o con estado
que difiere del inicial. `status p` siempre muestra esa página. El directorio es un
snapshot confirmado, no necesariamente el estado transitorio en plena transacción.

Una orden local pendiente impide otra read/write, pero permite status/help y cierre
pendiente. N0 también puede responder ERROR BUSY si otro nodo ya ocupa la transacción.
No hay reintentos automáticos de operaciones DSM.
