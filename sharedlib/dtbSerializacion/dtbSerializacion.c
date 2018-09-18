#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include "dtbSerializacion.h"

void myTrim(char *aDonde,char *contenidoACortar){
	char **splitClave;			// Esto se hace porque el trim no funciono
	splitClave = string_split(contenidoACortar," ");
	strcpy(aDonde,splitClave[0]);
    free(splitClave[0]);
    free(splitClave[1]);
    free(splitClave);
}

void imprimirDTB(DTB *miDTB){
	int cantElementos;

    myPuts("ID_GDT: %d Ruta_Escriptorio: %s PC: %d Flag_EstadoGDT: %d Lista:",miDTB->ID_GDT,miDTB->Escriptorio,miDTB->PC,miDTB->Flag_EstadoGDT);

    cantElementos = list_size(miDTB->tablaArchivosAbiertos);

    for(int indice = 0;indice < cantElementos;indice++){
		datosArchivo *misDatos;

		misDatos = list_get(miDTB->tablaArchivosAbiertos,indice);

    	myPuts("Elem%d Nombre_Archivo: %s Direccion_Memoria: %l \n",indice,misDatos->nombreArchivo,misDatos->dirMemoria);
    }
}


char* DTBStruct2String(DTB *miDTB){
	char* miStringDTB;

	int lenLista= list_size(miDTB->tablaArchivosAbiertos);

	miStringDTB = malloc(264 + (lenLista * (128 + 10)) + 1); // 264 = idGDT(2) + rutaScript(256) + PC(2) + estadoGDT(1) + lenLista(3)

	sprintf(miStringDTB,"%2d%-256s%2d%1d%03",miDTB->ID_GDT,miDTB->Escriptorio,miDTB->PC,miDTB->Flag_EstadoGDT,lenLista);
	//El %03 me dice que puede llegar a tener 999 archivos abiertos

	for(int indice = 0;indice < lenLista;indice++){
		datosArchivo *misDatos;

		misDatos = list_get(miDTB->tablaArchivosAbiertos,indice);

		strcat(miStringDTB,string_from_format("%128s",misDatos->nombreArchivo));
		strcat(miStringDTB,string_from_format("%10l",misDatos->dirMemoria));
	}
	miStringDTB[264 + (lenLista * (128 + 10))]='\0';

	return miStringDTB;
}

DTB* DTBString2Struct (char *miStringDTB){
	DTB *miDTB;

	char ID_GDT[3];
	char* Escriptorio;
	char PC[3];
	char Flag_EstadoGDT[2];
	char strLenLista[4];
	int lenLista;

	int nTotDesplaza = 0;
	int nProxLectura = 0;

	miDTB = malloc(sizeof(DTB));

	nProxLectura = 2;
	strncpy(ID_GDT,miStringDTB,nProxLectura);
	ID_GDT[2] = '\0';
	miDTB->ID_GDT = atoi(ID_GDT);
	nTotDesplaza += nProxLectura;

	nProxLectura = 256;
	Escriptorio = malloc(nProxLectura+1);
	strncpy(Escriptorio,miStringDTB + nTotDesplaza,nProxLectura);
	Escriptorio[nProxLectura] = '\0';
	nTotDesplaza += nProxLectura;

	myTrim(&(miDTB->Escriptorio),Escriptorio);
	/*
	char **splitClave;			// Esto se hace porque el trim no funciono
	splitClave = string_split(Escriptorio," ");
	strcpy(miDTB->Escriptorio,splitClave[0]);
    free(splitClave[0]);
    free(splitClave[1]);
    free(splitClave);
    */

	nProxLectura = 2;
    strncpy(PC,miStringDTB + nTotDesplaza,nProxLectura);
	PC[nProxLectura] = '\0';
	miDTB->PC = atoi(PC);
	nTotDesplaza += nProxLectura;

	nProxLectura = 1;
	strncpy(Flag_EstadoGDT,miStringDTB + nTotDesplaza,nProxLectura);
	Flag_EstadoGDT[nProxLectura] = '\0';
	miDTB->Flag_EstadoGDT = atoi(Flag_EstadoGDT);
	nTotDesplaza += nProxLectura;

	miDTB->tablaArchivosAbiertos = list_create();

	nProxLectura = 3;
	strncpy(strLenLista,miStringDTB + nTotDesplaza,nProxLectura);
	strLenLista[nProxLectura] = '\0';
	lenLista = atoi(strLenLista);
	nTotDesplaza += nProxLectura;

	if (lenLista != 0){
		int nInicioLista = miStringDTB + nTotDesplaza;

		for(int indice = 0;indice < lenLista;indice++){
			datosArchivo *misDatos;
			char nombre[128];
			char strMem[10];
			long mem;

			misDatos = malloc(sizeof(datosArchivo));

			strncpy(nombre,nInicioLista + ((128 + 10) * indice),128);
			strncpy(strMem,nInicioLista + ((128 + 10) * indice) + 128, 10);
			mem = atol(strMem);

			myTrim(&(misDatos->nombreArchivo),&nombre);
			misDatos->dirMemoria = mem;

			list_add(miDTB->tablaArchivosAbiertos,misDatos);
		}
	}

	free(Escriptorio);

	return miDTB;
}

/*DTB* recibirDTBNueva(int sock){
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
	buffer[45+lenValor] = '\0';
	return DTBString2Struct(buffer);
}

void enviarDTBNueva(int sock,DTB *miEntrada){
	char* strDTB;
	strDTB = DTBStruct2String (miEntrada);
	send(sock, strDTB, strlen(strDTB),0);
	//liberarDTBString(strDTB);
	free(strDTB);
}*/

