#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sentencias.h"

void imprimirSentencia(sentencia *miEntrada)
{
    if (miEntrada->operacion == OPERACION_SET){
    	myPuts("Operacion: %d Clave: %s Valor: %s\n",miEntrada->operacion, miEntrada->clave, miEntrada->valor);
    }
    else {
    	myPuts("Operacion: %d Clave: %s\n",miEntrada->operacion, miEntrada->clave);
    }
}

char* sentenciaStruct2String(sentencia *miEntrada){
	char* buffer;

	//buffer=malloc(45); //45= longitudes de:  operacion(1) + clave(40) + longitud valor(4)
	/*strcpy(buffer   ,string_itoa(miEntrada->operacion));
	strcpy(buffer+01,miEntrada->clave);
	strcat(buffer+41,miEntrada->valor);*/
	if(miEntrada->operacion == OPERACION_SET){
		int lenValor = strlen(miEntrada->valor);

		buffer=malloc(45+lenValor+1);

		sprintf(buffer,"%1d%-40s%04d",miEntrada->operacion,miEntrada->clave,lenValor); //sprintf() es como un printf pero para almacenar en un string y transforma entero
		strcat(buffer,miEntrada->valor);
		buffer[45+lenValor]='\0';
	}
	else {
		buffer=malloc(45+1); //45= longitudes de:  operacion(1) + clave(40) + longitud valor(4)
		sprintf(buffer,"%1d%-40s%04d",miEntrada->operacion,miEntrada->clave,0);
		buffer[45]='\0';
	}

	return buffer;
}

sentencia* recibirSentenciaNueva(int sock){
	char buffer[1024];
	int resultRecv=-1;
	resultRecv = myRecibirDatosFijos(sock,buffer,45);
	if (resultRecv!=0)
	{
		myPuts("Se deconecto el Socket %d de un proceso Esi!!\n", sock);

	}

	char strLenValor[5];
	strncpy(strLenValor,buffer+41,4);
	strLenValor[4] = '\0';
	int lenValor = atoi(strLenValor);
	if (lenValor != 0){ //Si es distinto de 0 es un SET porque hay valor
		resultRecv = myRecibirDatosFijos(sock,buffer+45,lenValor);
		if (resultRecv!=0)
		{
			myPuts("Se deconecto el Socket %d de un proceso Esi!!\n", sock);
		}
	}
	return sentenciaString2Struct(buffer);
}

void enviarSentenciaNueva(int sock,sentencia *miEntrada){
	char* strSentencia;
	strSentencia = sentenciaStruct2String (miEntrada);
	send(sock, strSentencia, strlen(strSentencia),0);
	//liberarSentenciaString(strSentencia);
	free(strSentencia);

}
sentencia* sentenciaString2Struct (char *miEntrada){
	sentencia *buffer;
	char operacion[2];
	char* strClave;
	char strLenValor[5];
	int lenValor;

	buffer = malloc(sizeof(sentencia));


	strncpy(operacion,miEntrada,1);
	operacion[1] = '\0';
	buffer->operacion = atoi(operacion);

	//strClave = malloc(41);
	strClave= malloc(41);
	strncpy(strClave,miEntrada+1,40);
	strClave[41] = '\0';
	char **splitClave;			// Esto lo hace porque el trim no funciono
	splitClave=string_split(strClave," ");
	strcpy(buffer->clave,splitClave[0]);

	buffer->valor = NULL;

	strncpy(strLenValor,miEntrada+41,4);
	strLenValor[4] = '\0';
	lenValor = atoi(strLenValor);
	if (lenValor != 0){
		char *strValor = malloc(lenValor+1);
		strncpy(strValor,miEntrada+45,lenValor);
		strValor[lenValor] = '\0';
		buffer->valor = strValor;
		//free(strValor);
	}

	free(strClave);
	return buffer;
}

void liberarSentenciaString (char *miEntrada) {
	//free(miEntrada);
}

void liberarSentenciaStruct (sentencia *miEntrada) {
	free(miEntrada);
}

