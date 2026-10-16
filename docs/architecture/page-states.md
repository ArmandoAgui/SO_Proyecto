# Estados y reglas de una página

| Estado local | Significado | Entrada | Salida |
|---|---|---|---|
| INVALID | No hay copia utilizable, aunque queden bytes viejos | Inicio N1; invalidación confirmada localmente | Instalar PAGE_DATA autorizado para lectura o escritura |
| READ_ONLY | Copia válida de la versión confirmada; no autoriza modificar | Lectura remota de un nodo no propietario | INVALIDATE o concesión exclusiva para escribir |
| OWNER | Copia autoritativa; es dueño en el directorio estable | Inicio N0 o transferencia de propiedad | Invalidación durante transferencia al otro nodo |

OWNER puede coexistir con READ_ONLY en el otro nodo. Por ello OWNER no equivale al
estado exclusivo E de MESI. Exclusividad se verifica con el directorio: `valid_mask`
contiene únicamente al dueño, además de una concesión WRITE activa. Se mantienen tres
estados educativos y una condición derivada; no se inventa un cuarto estado redundante.

## Transiciones válidas

| Desde | Evento | Hasta |
|---|---|---|
| INVALID | PAGE_DATA modo READ y autorización vigente | READ_ONLY |
| INVALID | PAGE_DATA modo WRITE, luego aplicación autorizada | OWNER |
| READ_ONLY | INVALIDATE | INVALID |
| READ_ONLY | GRANT WRITE después de invalidar al antiguo dueño | OWNER |
| OWNER | FETCH_PAGE para una lectura de otro nodo | OWNER |
| OWNER | INVALIDATE para ceder propiedad, tras preservar la instantánea | INVALID |
| OWNER | GRANT WRITE sin cambiar dueño | OWNER |
| READ_ONLY | Lectura autorizada de la misma versión | READ_ONLY |
| INVALID | INVALIDATE redundante en la transacción válida | INVALID |

Una lectura local de OWNER también lo mantiene OWNER. En la actualización de un
propietario con lectores, el dueño conserva OWNER y el lector pasa a INVALID.
No existe OWNER → READ_ONLY en este diseño: la transferencia de escritura invalida
al dueño anterior; podría leer más tarde y pasar INVALID → READ_ONLY.
No se permite INVALID → lectura directa ni READ_ONLY → escritura directa.

## Invariantes comprobables

1. En reposo, exactamente un OWNER por página y coincide con `directory.owner_id`.
2. Todo READ_ONLY tiene los mismos bytes y versión que el OWNER en reposo.
3. Para modificar: concesión WRITE vigente, destinatario único, invalidaciones
   confirmadas y ninguna otra operación en curso.
4. ACK de invalidación se envía después de cambiar a INVALID, nunca antes.
5. Una copia INVALID jamás satisface una lectura, aunque su versión parezca reciente.
6. El directorio solo confirma la nueva versión y propietario al recibir WRITE_DONE.

Durante una transferencia puede haber cero OWNER y estados aún por confirmar. La
transacción global bloquea nuevos accesos; no es una pérdida de datos porque existe
una instantánea y no se permite continuar tras fallos de transporte. `status` muestra
la fase pendiente y diferencia metadatos confirmados de estados locales transitorios.
