# Estructura preliminar del artículo IEEE

Esquema histórico. El artículo implementado está en paper.tex (IEEEtran); se genera
con make docs. Revisar el formato y los datos personales antes de entregar.

| Sección | Información que se debe recopilar durante el desarrollo |
|---|---|
| Abstract | Objetivo, método, alcance y resultados medidos; escribir al final |
| Keywords | DSM, page-based coherence, sockets, write-invalidate, operating systems |
| Introduction | Problema educativo, motivación, objetivos y contribuciones reales |
| Theoretical Background | DSM, paginación, coherencia frente a consistencia y fuentes primarias |
| System Architecture | Diagrama definitivo, responsabilidades, Linux/C y decisiones justificadas |
| DSM Protocol | Mensajes, estados, transiciones, invariantes, orden de acceso y casos de fallo |
| Implementation | Interfaces finales, estructuras, framing, bucle de eventos, versiones y commit |
| Testing | Entorno, herramientas, escenarios, metodología y reproducibilidad |
| Results | Datos reales: aciertos/fallos, transferencias, invalidaciones, tiempos y límites |
| Conclusions | Objetivos cumplidos, límites observados, aprendizaje y trabajo futuro |
| References | Referencias verificadas y efectivamente citadas, con metadatos bibliográficos |

Mantener un registro de decisiones y enlaces; guardar datos crudos antes de elaborar
figuras. No confundir fallos simulados con page faults de Linux. No generalizar el
rendimiento de dos procesos en loopback a un sistema distribuido de producción.
