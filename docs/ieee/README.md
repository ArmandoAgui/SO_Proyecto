# Artículo IEEE

paper.tex contiene el artículo en formato IEEEtran, con antecedentes citados,
arquitectura, implementación, metodología, resultados reales y limitaciones.

```sh
make docs
```

El PDF se genera en build/entrega/paper.pdf. Se compila dos veces para resolver
referencias; los logs de compilación quedan en esa carpeta. Se verificó con las
herramientas LaTeX ya instaladas, sin instalación adicional.

Antes de entregar: completar apellidos, universidad y sección; revisar el contenido,
confirmar que el formato satisface al docente y volver a ejecutar las pruebas si se
cambia código. No inventar autoría individual ni presentar las pruebas como garantía
formal. outline.md se conserva como esquema original de trabajo.
