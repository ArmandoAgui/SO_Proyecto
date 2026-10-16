# CLI propuesta

La CLI es futura. El esqueleto actual solo imprime su estado; no ejecuta estos comandos.

Arranque previsto (no disponible todavía):

```sh
./build/dsm-node --id 0 --listen 127.0.0.1 --port 5000
./build/dsm-node --id 1 --connect 127.0.0.1 --port 5000
```

| Orden | Efecto futuro |
|---|---|
| read <page> | Autorizar lectura, recuperar si INVALID y mostrar datos, versión y hit/fault |
| write <page> <data> | Obtener exclusividad y reemplazar el contenido completo de la página |
| status | Mostrar estado local, versión, owner_hint, orden pendiente y contadores; N0 también directorio |
| help | Mostrar sintaxis, límites y ejemplos |
| exit | Finalizar orden pendiente, cerrar socket y liberar recursos; el otro nodo termina al detectar cierre |

Páginas 0–255. Interpretar todo lo que sigue a `<page>` como texto (admite espacios,
sin sintaxis de shell ni comillas especiales); no aceptar texto vacío. Máximo 4095 bytes,
reservando NUL. Reemplazar la página con ceros y copiar el texto, no truncar silenciosamente.
En el modelo general la memoria son bytes; esta convención de texto simplifica la demo.
Mostrar cadenas con límite y escapar bytes no imprimibles. `status` muestra solo páginas
con actividad y acepta más adelante un filtro; no es un snapshot global durante una
transacción. No mostrar owner_hint como información autoritativa.

Números mediante strtoul con validación de rango, signo, fin de texto y errno. Una
orden mal formada no altera la memoria. Mientras hay una orden pendiente se permite
help/status; otras read/write reciben aviso local de ocupado. La recepción de red sigue
activa aunque stdin esté inactivo o haya una orden esperando.

Traza futura: `node=1 req=7 page=3 event=PAGE_FAULT state=INVALID version=0`.
Registrar SEND/RECV, INVALIDATE_APPLIED, GRANT_WRITE, OWNER_CHANGED y WRITE_DONE para
explicar causalidad. No usar tiempo de reloj como prueba del orden distribuido: usar
identidad de solicitud y fases del coordinador.
