# Plan de pruebas inicial

Estado: plan, no suite implementada. La compilación del esqueleto no valida DSM.

## Capas

- Build: `make clean && make`, advertencias activadas y ejecución del placeholder.
- Unitarias futuras en C: memoria/límites; codec y tamaños; parser; transiciones y
  fases con transporte simulado. No dependen de dos terminales ni de sleeps.
- Integración futura: lanzar ambos procesos, esperar HELLO con plazo y enviar órdenes
  sincronizadas por resultados/eventos, no por pausas arbitrarias. Usar Python estándar
  como harness si facilita controlar stdin/stdout, sin sustituir el simulador C.
- Manuales: demo con dos terminales y status en reposo; conservar trazas correlacionadas.

La suite de integración debe fallar si falta un evento, difiere el contenido, expira el
plazo o algún nodo termina inesperadamente. Limpiar procesos incluso si falla una aserción.

## Puertas de aceptación

1. Transporte: fragmentar cabecera y cuerpo, unir dos tramas, desconectar a mitad de una,
   rechazar longitud superior a página y HELLO incompatible; comprobar envío parcial.
2. Memoria: estados iniciales, primera/última página, rango fuera de límites, texto
   de 4095 bytes aceptado y de 4096 rechazado; lectura INVALID prohibida.
3. Protocolo: escenarios S01–S10 de scenarios.md; comprobar datos, versiones,
   propietario/máscara, orden INVALIDATED → GRANT WRITE → WRITE_DONE.
4. Concurrencia: solicitudes simultáneas producen una concesión y otra BUSY; reintento
   posterior usa versión vigente. Retrasar ACK deliberadamente y verificar que no hay
   escritura anticipada. Inyectar ACK con identidad/fase incorrecta: no desbloquea.
5. Recursos: cierre, EOF y timeout sin bloqueo indefinido. Sanitizers address/undefined
   durante integración, si el compilador los soporta; reportar herramienta y resultado.

Capturar por ejecución: fecha, commit, compilador/flags, Linux, configuración, órdenes,
salida esperada/real, estado PASS/FAIL y logs por nodo. No poner PASS sin ejecución.
No agregar un target `make test` vacío que termine con éxito. Añadirlo con pruebas reales.
