# Coherencia y consistencia — nota inicial

Coherencia de una página: las copias válidas no discrepan y escribir requiere invalidar
las otras copias. Consistencia: qué orden de accesos y valores observados permite el
sistema, incluso entre páginas distintas. No son sinónimos.

Nuestra propuesta ordena todos los accesos mediante N0 y no concede otro hasta recibir
la finalización del anterior. Esa serialización facilita justificar las observaciones
sin implementar un protocolo concurrente descentralizado. Es una decisión de diseño
pendiente de validar mediante código y pruebas, no un resultado ya demostrado.

Lectura teórica inicial: Adve y Gharachorloo, *Shared Memory Consistency Models: A Tutorial*,
Computer 29(12), 1996. [Referencia disponible en CMU](https://www.cs.cmu.edu/afs/cs/academic/class/15740-f18/www/papers/ieeemicro96-adve-consistency.pdf).
Ampliar y verificar metadatos bibliográficos antes de incorporarla al paper.
