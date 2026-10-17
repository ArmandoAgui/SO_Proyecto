# Pruebas de Fase 1

Ejecutar desde la raíz:

```sh
make test
```

Requisitos: compilador C11, Make y Python 3 (biblioteca estándar). El simulador no
depende de Python. Los puertos 5000 y 5001 deben estar libres para el escenario que
reproduce los comandos de la demo; los demás escenarios usan puertos temporales.

- `unit/network_test.c`: sockets locales de prueba para líneas fragmentadas y juntas,
  EOF normal y parcial, límite de longitud, envío a peer cerrado sin SIGPIPE y envío
  de 64 KiB con buffer pequeño a un proceso receptor que verifica todos los bytes.
  La prueba del buffer comprueba integridad; no instrumenta el número de llamadas send.
- `integration/test_sockets.py`: 13 casos con procesos reales y TCP loopback: ayuda;
  argumentos inválidos (13 variantes); N0 primero con reintento y puertos 5000/5001;
  N1 primero; Enter/exit/EOF; Ctrl+C; SIGTERM durante reintento y accept;
  puerto ocupado; HELLO fragmentado y EOF parcial; identidad HELLO inválida; timeout de HELLO y agotamiento de los 30 reintentos.
  La suite tarda aproximadamente 41 segundos.
  Comprueba el intercambio en ambos sentidos y que los procesos siguen vivos hasta
  el cierre. Los hijos se limpian aun cuando una prueba falla.

Estas pruebas no validan páginas ni consistencia DSM. Los escenarios futuros siguen
en `docs/testing/test-plan.md` y `scenarios.md`.
