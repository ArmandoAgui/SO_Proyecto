# Contribuciones

## Ramas y revisión

`main` conserva entregas verificadas; `develop` integra trabajo diario;
`feature/<issue>-<tema>` contiene una tarea breve. Flujo:

```text
feature/* --PR y revisión--> develop --PR de entrega--> main
```

El repositorio aún no tiene commits. Crear primero el commit de estructura en `main`,
publicarlo cuando el equipo lo decida y después crear `develop`. No se han creado ramas,
commits ni issues remotos automáticamente. Comprobar el remoto antes de publicar.

Para cada tarea, actualizar `develop`, abrir una rama, realizar cambios pequeños,
probarlos y abrir PR hacia `develop`. Una persona distinta revisa el contrato entre
módulos y los criterios del issue. Después de integrar, borrar la rama de trabajo.
Evitar cambios directos a `main`, force-push compartido y ramas de larga duración.
La PR de `develop` a `main` incluye pruebas de integración y guion de demo; etiquetar
la primera entrega completa como `v0.1.0`.

## Commits

`feat:`, `fix:`, `docs:`, `test:`, `refactor:` y, para herramientas, `build:` o `chore:`.
Ejemplos: `feat(network): recibir tramas completas`,
`fix(protocol): esperar ACK antes de conceder escritura`,
`docs(states): aclarar propietario con lectores`.

## Reglas de C para la implementación futura

C11; `-Wall -Wextra -Wpedantic -g`, incluir prototipos y verificar valores de retorno.
Sin variables globales mutables compartidas entre módulos; pasar contexto explícito.
Identificadores y código en inglés; explicación y documentación en español.
La CLI nunca escribe directamente el arreglo de memoria. Memory no usa sockets.
Protocol no conoce el formato textual de CLI. Network no cambia estados DSM.
No enviar estructuras C con `send(sizeof(struct))`: codificar campos explícitos.

Cada PR debe describir comportamiento, issue, pruebas ejecutadas y limitaciones.
Cambiar el protocolo exige actualizar su documento y escenarios antes de integrarlo.
Cada responsable técnico documenta su propio módulo y revisa otro.
