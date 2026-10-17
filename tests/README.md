# Pruebas del simulador

`make test` ejecuta tres programas C y 22 casos de integración usando solo Python
estándar como harness. Requiere puertos 5000/5001 libres y tarda alrededor de 50 s,
porque incluye agotamiento de reintentos y timeout de saludo.

| Archivo | Cobertura |
|---|---|
| unit/network_test.c | Líneas parciales/juntas, EOF, límite, SIGPIPE, envío íntegro, codec binario y cola EAGAIN. |
| unit/memory_cli_test.c | Memoria válida/inválida, versiones, límites y parser. |
| unit/protocol_test.c | Esperar ACK antes de escribir, ACK falso, BUSY y expiración. |
| integration/test_sockets.py | 13 casos de arranque, argumentos, conexión, cierre y timeout. |
| integration/test_dsm.py | Nueve casos de migración, copias, exclusividad, datos, errores y carreras controladas. |

La suite DSM incluye 48 operaciones con oráculo de valores y un peer falso que
retrasa ACK para comprobar el orden y provocar BUSY de manera determinista.
El harness tiene plazos y limpieza de hijos incluso si falla una aserción.

```sh
make test-unit
make test
make sanitize
# Solo DSM, con el ejecutable normal ya compilado:
python3 -m unittest discover -s tests/integration -p 'test_dsm.py' -v
```

DSM_BINARY permite elegir otro ejecutable sin sustituir el simulador por Python.
make sanitize usa un directorio build/sanitize separado y ASan/UBSan. Los mensajes de
error esperados de las pruebas negativas no significan fallo de la suite: evaluar
PASS/OK y el código de salida.

Ver docs/testing/RESULTADOS.md y evidence/ para resultados reales. Los escenarios
anteriores se conservan como referencia; la semántica vigente de read migra, mientras
read-copy conserva réplicas.
