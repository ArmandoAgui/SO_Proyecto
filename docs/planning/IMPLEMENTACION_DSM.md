# Implementación del simulador DSM

Se extendió la Fase 1 existente; se conservaron sus roles de conexión, validación de
argumentos, prefijo dsm_ y cinco módulos. El nuevo encargo del usuario autoriza avanzar
al proyecto completo; la restricción histórica de trabajar solo sockets ya no es el
alcance actual. No se han hecho commits, pushes ni entregas remotas automáticamente.

## Decisión motivada por la guía

La propuesta anterior replicaba todas las lecturas. La página 12 exige migrar e
invalidar al dueño al leer remotamente. Ahora read migra; read-copy conserva una
réplica para la demo de invalidación de lectores. Se documenta esta diferencia en vez
de presentar una réplica como migración. N0 sigue siendo coordinador y la propiedad
puede moverse a N1.

## Cambios de código

| Archivos | Cambio |
|---|---|
| src/memory/memory.c, include/dsm/memory.h | Arreglo, estados, lectura validada, instalación, invalidación y escritura. |
| src/protocol/protocol.c, include/dsm/protocol.h | Directorio, transacción, exclusividad, versiones, mensajes y contadores. |
| src/cli/cli.c, include/dsm/cli.h | Parser de comandos, ayuda y salida acotada/escapada. |
| src/network/network.c, include/dsm/network.h | Conservan sockets/saludo; agregan codec binario, cola y recepción parcial. |
| include/dsm/types.h | Constantes de geometría, mensajes, modos y fases ACK. |
| src/node/node.c | Integra memoria/protocolo, CLI incremental, colas y plazos en poll. |
| src/main.c | Mantiene argumentos y actualiza la ayuda de capacidades. |
| Makefile | Build configurable, pruebas, sanitizers, demo, PDFs y ZIP. |
| tests/unit/*, tests/integration/* | Conservan y amplían pruebas con datos, errores y carreras controladas. |
| tools/run_demo.py, tools/package.py | Evidencia reproducible y empaquetado estándar. |
| .gitignore | Cachés Python y auxiliares de LaTeX. |

La documentación activa en README, src/README, include/README, docs/architecture,
docs/testing y docs/research refleja el código. Se agregan guía sencilla, matriz de
requisitos, artículo IEEE, presentación y guion de video. Los planes originales y
FASE1_SOCKETS.md se conservan como historial y se señalan como tales.

## Resultado y validación

make desde limpio sin warnings. Tres programas de pruebas C y 22 pruebas de
integración aprobadas. Nueve escenarios DSM también aprobados con ASan/UBSan.
La demo automática registra cinco commits sobre página 3: escribir N0, migrar a N1,
replicar en N0, escribir N1 e invalidar N0, migrar de vuelta a N0. Ver trazas y huella
de fuentes en docs/testing/evidence/.

## Riesgos y límites

Modelo educativo explícito y volátil. El coordinador es un punto único de fallo.
No hay recuperación tras desconexión o timeout, ni garantías durante caídas. Las
pruebas no sustituyen una demostración formal. Los documentos requieren revisión del
equipo, y el video y la entrega externa deben realizarse por personas.
