# Entrega del Proyecto 10

Fuente del encargo: documento local `C06_ProyectoCatedraSO.pdf`, suministrado por el
usuario. Páginas 2 (instrucciones), 12 (Proyecto 10) y 13 (rúbrica). No se incorpora
el PDF del docente al ZIP del código. Sus instrucciones de entrega son requisitos
del curso, no autorización para acceder a Teams o entregar en ECampus.

## Correspondencia con los requisitos

| Requisito del PDF | Implementación/evidencia |
|---|---|
| Dos procesos conectados por sockets; misma máquina permitida | Dos ejecuciones C; TCP IPv4 loopback; prueba con 5000/5001. |
| Gran arreglo dividido en páginas | 1 MiB contiguo por nodo, 256 páginas de 4096 bytes. |
| Un proceso dueño de todas al inicio | N0 OWNER; N1 INVALID; prueba de inicialización y status. |
| Detectar página que no está localmente | PAGE_FAULT simulated=1; lectura INVALID rechazada por memory. |
| Solicitar al dueño por red | READ_REQUEST a N0; si N1 es dueño, FETCH_PAGE recupera sus bytes. |
| Dueño invalida y transfiere | read preserva instantánea, invalida y transmite PAGE_DATA completo. |
| Instalar y continuar | PAGE_INSTALLED, RESULT READ y ACK_DONE. |
| Propiedad exclusiva para escribir | Transacción y GRANT_WRITE después de todas las invalidaciones. |
| Invalidar otras copias, si se permiten lectores | read-copy produce READ_ONLY; INVALIDATE/ACK y prueba de escritura retrasada. |
| Demo 1: migración para lectura | N0 write 3; N1 read 3; estados INVALID/OWNER. |
| Demo 2: escritura invalida copia previa | N0 read-copy 3; N1 write 3; N1 envía INVALIDATE a N0 antes de escribir. |
| Investigación y artículo IEEE | docs/research/FUENTES.md y docs/ieee/paper.tex; PDF con make docs. |
| Presentación ejecutiva | presentation/presentacion.tex; PDF con make docs. |
| Código fuente | src, include, Makefile, tests y tools. |
| Video explicativo | Guion en demo/README.md; debe grabarlo y revisarlo el equipo. |

## Revisión de la entrega

```sh
make clean
make
make test
make sanitize
make demo
make docs
make package
```

El ZIP está en `build/entrega/proyecto10-dsm.zip`. Dentro, los PDFs están en
`proyecto10-dsm/entregables/`. Incluye fuentes y PDFs, sin binarios
compilados ni dependencias privadas del equipo. El receptor puede ejecutar make.
No contiene un video inventado ni un archivo que lo suplante.

## Lo que debe completar el equipo

- Confirmar apellidos, universidad y sección. Los primeros nombres proporcionados son
  Armando, Daniel, Guillermo, Daniel y Mauricio; se conservan ambos Daniel.
  Docente: David Garcia Cerna. No se inventan matrículas, correos ni contribuciones.
- Revisar el artículo y las diapositivas, especialmente las limitaciones y resultados.
- Practicar la demo, estudiar las preguntas y asignarse la exposición según su trabajo real.
- Grabar un video con explicación y ejecución verificable, revisar imagen/audio y
  entregar el archivo/enlace según indique el docente.
- Subir los entregables a ECampus y comprobar el comprobante de envío.

El documento indica entrega el **10 de noviembre, 12:00 del mediodía**, y
presentaciones el 12 y 19 de noviembre, 18:30–20:10. No especifica año en esas líneas;
el equipo debe confirmar el calendario vigente y su grupo en ECampus o con el docente.
No se ha programado ni enviado ningún mensaje a Teams ni ninguna entrega externa.

## Requisitos en Pop!_OS

Ya disponibles en el entorno verificado: GCC 13.3, Make, Python 3, pdflatex,
IEEEtran, Beamer y soporte de español. No se instaló software ni se necesitó sudo.

En una instalación nueva, solo si faltan herramientas:

```sh
sudo apt update
sudo apt install build-essential python3
# Opcional: únicamente para regenerar los PDFs desde LaTeX:
sudo apt install texlive-latex-extra texlive-publishers texlive-lang-spanish
```

Los comandos anteriores están documentados para otra instalación; no se ejecutaron
automáticamente. Los PDFs generados pueden abrirse sin instalar LaTeX.
No requiere cambios de kernel, Docker, VMs, privilegios especiales ni bibliotecas C externas.
