# Código inicial

main.c ofrece --help e informa que no existe simulación. node/node.c contiene el stub
de ejecución; network, memory, protocol y cli tienen stubs de inicialización que
retornan DSM_NOT_IMPLEMENTED. No reservan memoria DSM, abren sockets ni cambian estados.
Los contratos detallados futuros están en docs/architecture/. Los stubs no son la API
definitiva: sustituirlos por interfaces con contexto conforme avance cada fase.
