# Demo y guion del video

El simulador está implementado. Antes de grabar, practicar los comandos y verificar
que los puertos 5000/5001 estén libres. No ejecutar make test al mismo tiempo que la
demo manual porque ese escenario usa los mismos puertos.

## Preparación

```sh
make
# En terminal 1:
./build/dsm-node --id 0 --port 5000 --peer-port 5001
# En terminal 2:
./build/dsm-node --id 1 --port 5001 --peer-port 5000
```

Colocar terminales lado a lado con letra grande y rótulos N0/N1. Esperar DSM_READY.
Usar una línea a la vez; esperar RESULT antes de la siguiente.

## Guion de 8–10 minutos

1. Presentar el problema y los cinco integrantes (30–45 s). Explicar que son dos
   procesos y dos arreglos; las transferencias TCP sí son reales.
2. Mostrar arquitectura y etiquetas (1–2 min). Distinguir coordinador de propietario
   y fallo simulado de fallo real del kernel.
3. Mostrar `status 3` en ambos: N0 OWNER y N1 INVALID, versión 0.
4. N0: `write 3 hola desde N0`; versión 1. N1: `read 3`; señalar PAGE_FAULT,
   PAGE_DATA, N0 INVALID y N1 OWNER. Es la migración solicitada en el PDF (2 min).
5. N0: `read-copy 3`; señalar READ_ONLY y explicar que esta orden conserva réplica.
   N1: `write 3 actualizado por N1`; señalar SEND INVALIDATE en N1,
   INVALIDATE_APPLIED y GRANT_WRITE en N0, después RESULT WRITE y COMMIT (2 min).
6. N0: `read 3`; comprobar texto nuevo y migración de regreso. Mostrar status.
7. Explicar la prueba inversa: con dueño N0 y lector N1, N0 espera ACK remoto
   antes de escribir. La suite inyecta retraso para probarlo, no solo observar logs.
8. Introducir `read 256`: el error no cambia memoria. Cerrar con exit.
9. Mostrar `make test` ya terminado o ejecutar la suite completa (tarda ~50 s).
   Terminar con límites y trabajo futuro (1–2 min).

La demo automática también está disponible:

```sh
make demo
```

Guarda comandos, trazas y huella de fuentes en docs/testing/evidence. No graba video.
La evidencia muestra una ejecución real; no reemplaza una explicación oral.

## Preguntas para practicar

- ¿Por qué dos procesos en la misma máquina simulan nodos? Porque tienen memoria
  independiente y solo intercambian páginas mediante sockets.
- ¿Dónde ocurre el fallo de página? En el chequeo de INVALID del acceso explícito,
  no mediante SIGSEGV ni una interrupción del kernel.
- ¿Por qué read y read-copy difieren? Para cumplir migración e ilustrar lectores.
- ¿Por qué no basta con ser OWNER para escribir? Puede existir un lector READ_ONLY.
- ¿Cuándo se confirma una invalidación? Después de cambiar el estado a INVALID.
- ¿Qué hace N0 si el dueño es N1? Recupera bytes con FETCH_PAGE antes de invalidar.
- ¿Cómo se evita una escritura antes de los ACK? La transacción permanece INVALIDATING.
- ¿Qué pasa si dos escriben a la vez? N0 admite una; otra recibe BUSY o espera por
  el orden de llegada. No hay dos concesiones globales activas.
- ¿Por qué un send no equivale a un mensaje? TCP es flujo; hay framing y offsets.
- ¿Qué pasa si cae N0? Se aborta; el coordinador es un punto único de fallo.

## Grabación y entrega

El equipo debe usar la herramienta de captura de pantalla que tenga disponible en
su escritorio, mostrar terminales/diapositivas y narrar. Comprobar micrófono y texto
legible antes de grabar. No se instaló un grabador ni se modificaron permisos del
escritorio. Exportar y revisar el video completo, y entregarlo en el formato pedido.
Completar nombres/apellidos y verificar el plazo en ECampus antes de subirlo.
