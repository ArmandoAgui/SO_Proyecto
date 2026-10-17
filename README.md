# Mini-simulador DSM a nivel de página

Proyecto 10 de Sistemas Operativos. C11 en Linux/Pop!_OS, dos procesos independientes,
TCP IPv4 sobre `127.0.0.1`, sin hilos ni bibliotecas externas de ejecución.

**Implementado:** 1 MiB por nodo, 256 páginas de 4096 bytes, migración para lectura,
copias de solo lectura opcionales, propiedad exclusiva para escritura, invalidaciones
confirmadas, versiones, directorio central, CLI, pruebas y trazas. El fallo de página
se simula al comprobar metadatos; no se intercepta SIGSEGV ni se modifica el kernel.

Equipo: Armando, Daniel, Guillermo, Daniel y Mauricio. Catedrático: David Garcia Cerna.
Faltan apellidos, universidad y sección para personalizar los documentos definitivos.

## Empezar en Pop!_OS

En el equipo de desarrollo ya están instalados GCC, Make, Python 3 y LaTeX. Para
compilar y ejecutar solo se requieren GCC/Clang y Make. Python 3 se usa en pruebas,
demo automática y empaquetado. No hace falta Docker, máquinas virtuales, MPI ni sudo.

```sh
make
```

Terminal 1, desde la raíz del repositorio:

```sh
./build/dsm-node --id 0 --port 5000 --peer-port 5001
```

Terminal 2, en la misma carpeta:

```sh
./build/dsm-node --id 1 --port 5001 --peer-port 5000
```

Ambos escuchan; N0 conecta y N1 acepta una sola conexión bidireccional. Cualquiera
puede iniciar primero. N0 hace hasta 30 intentos con pausas de 1 s. Espera a ver
`DSM_READY` en ambos antes de escribir comandos. Los puertos deben estar libres y
el peer-port de cada nodo debe coincidir con el port del otro.

## Demo del documento de la asignatura

Ejecuta **una línea a la vez**, esperando su resultado, en la terminal indicada:

| Paso | Terminal | Comando | Qué demuestra |
|---|---|---|---|
| 1 | N0 | `write 3 hola desde N0` | N0 posee la página y escribe versión 1. |
| 2 | N1 | `read 3` | Fallo simulado, transferencia de 4096 bytes y migración N0 → N1. |
| 3 | N0 | `status 3` | Copia INVALID; directorio con propietario 1. |
| 4 | N1 | `status 3` | Copia OWNER, versión 1. |
| 5 | N0 | `read-copy 3` | Copia READ_ONLY; N1 conserva propiedad. |
| 6 | N1 | `write 3 actualizado por N1` | Envía INVALIDATE a N0; N0 invalida antes de conceder la escritura. |
| 7 | N0 | `status 3` | La copia de lectura vuelve a INVALID. |
| 8 | N0 | `read 3` | Nuevo fallo; obtiene el texto actualizado y recupera la propiedad. |

`read` migra para cumplir el apartado a–c de la guía. `read-copy` es una operación
separada que permite demostrar la invalidación de lectores prevista en esa misma
guía. No confundir ambas: solo `read-copy` conserva una copia en el dueño anterior.

Busca `PAGE_FAULT`, `PAGE_DATA`, `INVALIDATE_APPLIED`, `GRANT_WRITE`, `COMMIT` y
`OWNER_CHANGED`. Al invalidar una copia de N1, N0 envía INVALIDATE y espera ACK.
Al invalidar una copia local de N0, el coordinador aplica la invalidación directamente;
si N1 era dueño y quiere escribir, envía INVALIDATE como solicitud de exclusividad.

## Comandos

| Orden | Efecto |
|---|---|
| `read <página>` | Lee y deja un único dueño; migra propiedad si hace falta. |
| `read-copy <página>` | Lee sin migrar, conservando una copia de lectura si no es dueño. |
| `write <página> <texto>` | Obtiene exclusividad y reemplaza toda la página. |
| `status [página]` | Estados/versiones locales y contadores; N0 también muestra directorio. |
| `help` | Ayuda de comandos. |
| `exit` o Enter | Espera la operación en curso, cierra y libera recursos. |

Páginas 0–255. Texto de 1 a 4095 bytes, con espacios, sin interpretación de comillas
ni shell. Una línea inválida no modifica memoria. Cada nodo admite una solicitud local
pendiente. N0 ordena una operación global a la vez; si otra llega durante ella, devuelve
BUSY y se reintenta manualmente. `help` y `status` siguen disponibles mientras se espera.

EOF en stdin solicita cierre; Ctrl+C/SIGTERM abortan la sesión y liberan recursos.
El peer termina al detectar cierre. Un fallo a mitad de operación **aborta** la sesión;
no se promete recuperación de propiedad. Reinicia ambos nodos para empezar de cero.

## Verificar y preparar la entrega

```sh
make test       # Tres ejecutables de pruebas C + 22 pruebas de integración (~50 s)
make sanitize   # AddressSanitizer/UndefinedBehaviorSanitizer + nueve pruebas DSM
make demo       # Repite y verifica la demo; guarda evidencia real
make docs       # Artículo IEEE y presentación PDF (requiere LaTeX)
make package    # Fuentes y PDFs en build/entrega/proyecto10-dsm.zip
make help
```

`make test` utiliza 5000/5001 en un caso: cierra la demo manual antes de lanzarlo.
Los demás escenarios usan puertos efímeros. `make clean` elimina build, incluidos
PDFs y ZIP; `make docs` y `make package` los generan de nuevo.

- [Guía sencilla: cómo funciona](docs/GUIA_SENCILLA.md).
- [Requisitos del PDF y pendientes de entrega](docs/planning/ENTREGA_PROYECTO10.md).
- [Arquitectura](docs/architecture/architecture.md), [protocolo](docs/architecture/protocol.md),
  [datos](docs/architecture/data-model.md), [estados](docs/architecture/page-states.md).
- [Guion de demo/video](demo/README.md) y [presentación](presentation/README.md).
- [Artículo IEEE](docs/ieee/paper.tex) y [fuentes verificadas](docs/research/FUENTES.md).
- [Pruebas y resultados](docs/testing/RESULTADOS.md).
- [Reporte de implementación](docs/planning/IMPLEMENTACION_DSM.md).

## Límites

Dos nodos, loopback, memoria volátil, una operación global activa y páginas completas.
Sin acceso transparente mediante punteros, mmap/mprotect, señales de fallo reales,
recuperación, persistencia, autenticación, reconexión ni escalabilidad multinodo.
La serialización facilita justificar el orden de accesos completados durante una sesión
sin fallos; las pruebas aportan evidencia, no una verificación formal exhaustiva.

El código y los materiales están preparados para revisión. El equipo debe completar
datos personales, estudiar y grabar la explicación, revisar los documentos y entregar
los archivos en ECampus. No se ha grabado ni enviado un video automáticamente.
