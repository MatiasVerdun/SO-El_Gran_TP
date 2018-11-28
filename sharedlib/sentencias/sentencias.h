#include <stdio.h>
#include <stdlib.h>

enum { OPERACION_DUMMY,
	OPERACION_ABRIR,
	OPERACION_CONCENTRAR,
	OPERACION_ASIGNAR,
	OPERACION_WAIT,
	OPERACION_SIGNAL,
	OPERACION_FLUSH,
	OPERACION_CLOSE,
	OPERACION_CREAR,
	OPERACION_BORRAR,
	};

enum{ DESCONEXION_CPU,
	ACC_DUMMY_OK,
	ACC_DUMMY_ERROR,
	ACC_CREAR_OK,
	ACC_CREAR_ERROR,
	ACC_BORRAR_OK,
	ACC_BORRAR_ERROR,
	ACC_ABRIR_OK,
	ACC_ABRIR_ERROR,
	ACC_FLUSH_OK,
	ACC_FLUSH_ERROR,
};

typedef struct sentencia {
	int operacion;
	char *param1;
	int param2;
	char *param3;
} sentencia;


void imprimirSentencia(sentencia *miEntrada)
{
    if (miEntrada->operacion == OPERACION_ASIGNAR){
    	myPuts("Operacion: %d Param1: %s Param2: %d Param3: %s\n",miEntrada->operacion,miEntrada->param1, miEntrada->param2, miEntrada->param3);
    }
    else if (miEntrada->operacion == OPERACION_CREAR){
        	myPuts("Operacion: %d Param1: %s Param2: %d\n",miEntrada->operacion, miEntrada->param1, miEntrada->param2);
    }
    else if (miEntrada->operacion == OPERACION_CONCENTRAR){
        	myPuts("Operacion: %d\n",miEntrada->operacion);
    }
    else {
    	myPuts("Operacion: %d Param1: %s\n",miEntrada->operacion, miEntrada->param1);
    }
}
