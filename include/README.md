# Interfaces del simulador

- dsm/types.h: geometría, estados de resultado y mensajes lógicos compartidos.
- dsm/node.h: configuración y ejecución del nodo.
- dsm/network.h: sockets, saludo y buffers del transporte binario.
- dsm/memory.h: páginas y operaciones locales comprobadas.
- dsm/protocol.h: directorio, transacción, contexto y despacho.
- dsm/cli.h: comandos y salida acotada.

Los encabezados definen representación interna C. El formato de red se codifica
explícitamente en network; no depende de sizeof de ninguna estructura.
