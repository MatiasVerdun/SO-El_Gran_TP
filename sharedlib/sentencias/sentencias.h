#include <stdio.h>
#include <stdlib.h>

enum {	OPERACION_CERO,
	OPERACION_ABRIR,
	OPERACION_CONCENTRAR,
	OPERACION_ASIGNAR,
	OPERACION_WAIT,
	OPERACION_SIGNAL,
	OPERACION_FLUSH,
	OPERACION_CLOSE,
	OPERACION_CREAR,
	OPERACION_BORRAR};

typedef struct sentencia {
	int operacion;
	char *param1;
	int param2;
	char *param3;
} sentencia;
