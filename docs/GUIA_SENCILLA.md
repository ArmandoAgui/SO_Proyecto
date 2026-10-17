# Cómo funciona, en palabras sencillas

El programa se abre dos veces. Cada proceso tiene su propia memoria y no puede leer
la del otro directamente. Los sockets son el canal para pedirse páginas y enviarlas.
No importa que los procesos estén en una sola computadora: sus arreglos son separados.

## Memoria dividida en páginas

Piensa en un cuaderno de 256 hojas. Cada hoja es una página de 4096 bytes.
Al comenzar, N0 tiene las 256 hojas válidas, vacías y bajo su propiedad. N1 tiene
espacio reservado, pero sus hojas están marcadas INVALID: sus bytes no se pueden usar.

Cada página lleva una etiqueta:

- OWNER: aquí está la copia autorizada. Ser dueño no permite escribir sin coordinación.
- READ_ONLY: copia que permite leer, pero requiere exclusividad para modificar.
- INVALID: los bytes que puedan quedar aquí ya no se deben leer.

## Leer: pedir una hoja que no tienes

Si N1 escribe `read 3`, comprueba su etiqueta. Como no tiene la página, registra un
fallo de página **simulado** y manda READ_REQUEST. El dueño guarda el contenido para
transferirlo, invalida su copia y el solicitante recibe los 4096 bytes. N1 instala la
página y muestra el texto. La propiedad ha migrado. Linux no está detectando un fallo:
lo detecta nuestra función de acceso al revisar INVALID.

`read-copy 3` muestra una variante: deja al dueño con su página y al solicitante con
una copia de solo lectura. Sirve para enseñar por qué luego hay que invalidar lectores.

## Escribir: obtener permiso antes de cambiar datos

N0 mantiene una lista que dice quién es dueño de cada página y quién tiene copia válida.
Esta lista se llama directorio. N0 coordina una operación a la vez, aunque los datos
pueden pertenecer a N1. Coordinador y propietario no significan lo mismo.

Para escribir:

1. Se solicita exclusividad.
2. Si faltan los datos, se recupera la página completa antes de invalidar al dueño.
3. Se invalidan las otras copias. Una invalidación remota necesita confirmación ACK;
   la invalidación local del coordinador se aplica directamente.
4. Solo después se concede el permiso y se reemplaza la página con el texto nuevo.
5. Se aumenta su versión y se confirma la operación en el directorio.

Así se evita que otro nodo lea una copia vieja como si fuera válida. Cuando vuelva a
necesitarla, tendrá que pedir la versión actualizada.

## Qué ver en la consola

| Mensaje | Explicación |
|---|---|
| DSM_READY | Ambos conocen el número y tamaño de páginas. |
| PAGE_FAULT simulated=1 | El programa detectó una copia INVALID. |
| PAGE_DATA | Viaja una página completa por TCP. |
| INVALIDATE / INVALIDATE_APPLIED | Se pide / se aplica dejar de usar una copia. |
| ACK | Se confirma el paso anterior. |
| GRANT_WRITE | Ya se puede escribir con exclusividad. |
| RESULT READ / WRITE | Resultado de la operación del usuario. |
| COMMIT | El directorio confirma la operación terminada. |
| OWNER_CHANGED | La propiedad pasó a otro nodo. |
| BUSY | Otra operación está en curso; hay que reintentar después. |

`req` identifica una solicitud dentro de un nodo; usa junto con `origin` porque ambos
nodos pueden tener una solicitud número 1. `version` aumenta al escribir, no al copiar.

## Cómo organiza el trabajo el código

- main lee los argumentos de arranque.
- node conecta todo y atiende terminal/red con poll.
- network convierte mensajes en bytes y los envía o acumula sin bloquear el bucle.
- memory guarda páginas y aplica cambios locales.
- protocol decide permisos, mensajes y orden.
- cli entiende comandos y muestra texto de forma segura.

Un envío TCP puede llegar en pedazos. Los primeros 40 bytes indican el tipo y cuánto
contenido sigue. El receptor acumula hasta tener la trama completa. La cola de envío
recuerda cuántos bytes faltan si el socket no puede aceptar todo de una vez.

## Qué no demuestra este proyecto

No es memoria realmente compartida por el kernel y no intercepta cualquier puntero C.
No tolera la caída de un nodo y no guarda datos al cerrar. Es un simulador didáctico
para observar el movimiento de páginas y las decisiones de consistencia.

Para repetirlo, sigue los comandos del README. Espera el resultado de cada comando
antes de introducir el siguiente. Si una sesión falla, reinicia ambos procesos.
