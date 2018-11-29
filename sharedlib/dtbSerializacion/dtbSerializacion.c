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

DTB* recibirDTB(int socket){
	DTB *miDTB;
	int resultRecv;
	int lenLista;
	char buffer[1024];
	char strLenLista[4];

	resultRecv = myRecibirDatosFijos(socket,buffer,265);
	if (resultRecv!=0)
	{
		myPuts("Se deconecto el S-AFA!!!\n");
		exit(1);
	}
	strncpy(strLenLista,buffer + 262,3);
	strLenLista[3] = '\0';
	lenLista = atoi(strLenLista);
	if (lenLista != 0){
		resultRecv = myRecibirDatosFijos(socket,buffer + 265,lenLista * (256 + 4 ));
		if (resultRecv!=0)
		{
			myPuts("Se deconecto el S-AFA!!!\n");
			exit(1);
		}
	}

	miDTB = DTBString2Struct(buffer);

	return miDTB;
}

void imprimirDTB(DTB *miDTB){
	int cantElementos;

    myPuts("ID_GDT: %d Ruta_Escriptorio: %s PC: %d Flag_EstadoGDT: %d Finalizo: %d Lista: ",miDTB->ID_GDT,miDTB->Escriptorio,miDTB->PC,miDTB->Flag_GDTInicializado,miDTB->totalDeSentenciasAEjecutar);

    cantElementos = list_size(miDTB->tablaArchivosAbiertos);

    if(cantElementos == 0){
    	myPuts("No hay elementos en la lista de Archivos Abiertos\n");
    } else {
        for(int indice = 0;indice < cantElementos;indice++){
    		datosArchivo *misDatos;

    		misDatos = list_get(miDTB->tablaArchivosAbiertos,indice);

        	myPuts("Elem %d Path_Archivo: %s Direccion_Memoria: %d \n",indice,misDatos->pathArchivo,misDatos->fileID);
        }
    }
}

char* DTBStruct2String(DTB *miDTB){
	char* miStringDTB;

	int lenLista= list_size(miDTB->tablaArchivosAbiertos);

	miStringDTB = malloc(265 + (lenLista * (256 + 4)) + 1); // 264 = idGDT(2) + rutaScript(256) + PC(2) + estadoGDT(1) + lenLista(3) + terminoEjecucion(1)

	sprintf(miStringDTB,"%02d%-256s%2d%1d%1d%03d",miDTB->ID_GDT,miDTB->Escriptorio,miDTB->PC,miDTB->Flag_GDTInicializado,miDTB->totalDeSentenciasAEjecutar,lenLista);
	//El %03 me dice que puede llegar a tener 999 archivos abiertos

	for(int indice = 0;indice < lenLista;indice++){
		datosArchivo *misDatos;

		misDatos = list_get(miDTB->tablaArchivosAbiertos,indice);
		char* pathArchivo=string_from_format("%256s",misDatos->pathArchivo);
		char* fileID=string_from_format("%4d",misDatos->fileID);

		strcat(miStringDTB,pathArchivo);
		strcat(miStringDTB,fileID);

		free(pathArchivo);
		free(fileID);

	}
	miStringDTB[265 + (lenLista * (256 + 4))]='\0';

	return miStringDTB;
}

DTB* DTBString2Struct (char *miStringDTB){
	DTB *miDTB;

	char ID_GDT[3];
	char* Escriptorio;
	char PC[3];
	char Flag_EstadoGDT[2];
	char strLenLista[4];
	char terminoEjecucion[2];
	int lenLista;

	int nTotDesplaza = 0;
	int nProxLectura = 0;

	miDTB = malloc(sizeof(DTB));

	nProxLectura = 2;
	strncpy(ID_GDT,miStringDTB,nProxLectura); //TODO
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
	miDTB->Flag_GDTInicializado = atoi(Flag_EstadoGDT);
	nTotDesplaza += nProxLectura;

	nProxLectura = 1;
	strncpy(terminoEjecucion,miStringDTB + nTotDesplaza,nProxLectura);
	terminoEjecucion[nProxLectura] = '\0';
	miDTB->totalDeSentenciasAEjecutar = atoi(terminoEjecucion);
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
			char path[256];
			char fileID[10];
			int id;

			memset(fileID,'\0',10);
			misDatos = malloc(sizeof(datosArchivo));

			strncpy(path,nInicioLista + ((256 + 4) * indice),256);
			strncpy(fileID,nInicioLista + ((256 + 4) * indice) + 256, 4);
			id = atoi(fileID);

			myTrim(&(misDatos->pathArchivo),&path);
			misDatos->fileID = id;

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
