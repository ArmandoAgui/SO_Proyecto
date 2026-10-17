# Responsabilidades para cinco integrantes

> Plan original conservado como historial. Para el estado actual, consulte
> `IMPLEMENTACION_DSM.md`, `ENTREGA_PROYECTO10.md` y el README de la raíz.

| Integrante | Responsabilidad técnica principal | Entregables y colaboración |
|---|---|---|
| I1 | Arquitectura e integración: node, main, configuración y bucle poll | Makefile, ciclo de vida, integración de módulos; revisa contrato y fallos con I4 |
| I2 | Sockets/networking | Codec, framing, envío/recepción parcial, HELLO, pruebas de transporte; investiga sockets |
| I3 | Gestión de memoria | Arreglo, Page, límites, instalación e invalidación local, pruebas unitarias; investiga paginación |
| I4 | Protocolo DSM y consistencia | Directorio, autorización, transacciones, invalidación y propiedad; pruebas de invariantes con I3 |
| I5 | CLI, pruebas y demo | Parser y presentación, automatización de dos procesos, guion; verifica integración con I1 |

Todos implementan y prueban código. Cada uno documenta su área, aporta evidencia al
artículo y explica parte de la demo. I1 no debe acaparar cambios: acuerda encabezados
antes de repartir implementaciones; cada módulo tiene un autor principal y un revisor.

Inicio paralelo: I1 estructura/build; I2 contrato TCP; I3 modelo de memoria; I4 tablas
de transiciones y secuencias; I5 casos de aceptación y parser. Integrar incrementos
pequeños con dependencias explícitas en los issues; no esperar cinco módulos completos.
