# Comenzar hoy

> Plan original conservado como historial. Para el estado actual, consulte
> `IMPLEMENTACION_DSM.md`, `ENTREGA_PROYECTO10.md` y el README de la raíz.

## Lista ordenada

1. Revisar en equipo alcance, tres estados, coordinador N0 y limitaciones. Acordar con
   el docente que fallos simulados y accesos explícitos cumplen el enunciado.
2. Ejecutar `make`, `./build/dsm-node --help` y `make run`: solo validan el esqueleto.
3. Asignar I1–I5 a personas y revisar encabezados. Son stubs, no API definitiva.
4. Revisar estos archivos iniciales: README, CONTRIBUTING, Makefile, types.h y el
   contrato de estados/protocolo. Resolver formato exacto de flags/códigos en G02.
5. Crear el primer commit de estructura en main y después develop cuando el equipo
   lo decida; verificar la configuración del remoto antes de hacer push.
6. Crear primero issues G01, G02, G03, G05 y G11 (alcance inicial: parser).
   Los borradores están en issues.md; no hay issues publicados automáticamente.
7. Comenzar fase 1 con conexión TCP y HELLO, sin páginas ni invalidaciones todavía.
   En paralelo se puede implementar y probar memoria local (fase 2).

La fase 0 tiene su estructura compilable. Falta la revisión y publicación que acuerde
el equipo. No marcar los módulos como funcionales por el hecho de compilar.

## Qué no intentar todavía

Protocolo DSM completo; SIGSEGV/mprotect; acceso transparente con punteros; tres o más
nodos; multihilo; réplica del coordinador; reconexión/recuperación; disco/swap/LRU;
GUI; optimización del protocolo; paper y diapositivas finales. Primero un hito verificable.

## Definición del primer MVP

Dos procesos conectados deben completar lectura local, fallo remoto, réplica de lectura,
escritura exclusiva con invalidación confirmada, transferencia de propiedad y relectura
de datos actualizados. Los estados/versiones deben concordar y las pruebas de integración
pasar. La secuencia exacta está en README.md y scenarios.md. Este esqueleto todavía no
cumple ese criterio.

## Árbol completo inicial

Se excluyen .git/ y build/ (metadatos y productos generados).

```text
SO_Proyecto/
├── demo/
│   └── README.md
├── docs/
│   ├── architecture/
│   │   ├── architecture.md
│   │   ├── cli.md
│   │   ├── data-model.md
│   │   ├── page-states.md
│   │   └── protocol.md
│   ├── ieee/
│   │   └── outline.md
│   ├── planning/
│   │   ├── issues.md
│   │   ├── roadmap.md
│   │   ├── start-today.md
│   │   └── team.md
│   ├── research/
│   │   ├── consistency.md
│   │   ├── dsm.md
│   │   ├── paging.md
│   │   └── sockets.md
│   └── testing/
│       ├── scenarios.md
│       └── test-plan.md
├── include/
│   ├── dsm/
│   │   ├── cli.h
│   │   ├── memory.h
│   │   ├── network.h
│   │   ├── node.h
│   │   ├── protocol.h
│   │   └── types.h
│   └── README.md
├── presentation/
│   └── README.md
├── src/
│   ├── cli/
│   │   └── cli.c
│   ├── memory/
│   │   └── memory.c
│   ├── network/
│   │   └── network.c
│   ├── node/
│   │   └── node.c
│   ├── protocol/
│   │   └── protocol.c
│   ├── README.md
│   └── main.c
├── tests/
│   ├── integration/
│   │   └── .gitkeep
│   ├── unit/
│   │   └── .gitkeep
│   └── README.md
├── .gitignore
├── CONTRIBUTING.md
├── Makefile
└── README.md
```
