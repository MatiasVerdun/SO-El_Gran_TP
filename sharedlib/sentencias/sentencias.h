#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int OPERACION_GET = 1;
const int OPERACION_SET = 2;
const int OPERACION_STORE = 3;
const int OPERACION_COMPACTACION = 4;
const int OPERACION_DUMP = 5;

typedef struct sentencia {
	int operacion;
	char clave[40];
	char *valor;
} sentencia;

char* sentenciaStruct2String(sentencia *miEntrada);
sentencia* sentenciaString2Struct (char *miEntrada);
void liberarSentenciaString (char *miEntrada);
void liberarSentenciaStruct (sentencia *miEntrada);

void imprimirSentencia(sentencia *miEntrada);
sentencia* recibirSentenciaNueva(int sock);
void enviarSentenciaNueva(int sock,sentencia *miEntrada);
