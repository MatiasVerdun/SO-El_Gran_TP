#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <conexiones/mySockets.h>
#include <console/myConsole.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <archivos/archivos.h>
#include "filesystemFIFA.h"
#include "interfaz.h"


t_config *configMDJ;

///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("DAM -> IP: %s - Puerto: %s \0", (char*)getConfigR("IP_ESCUCHA",0,configMDJ),(char*)getConfigR("DAM_PUERTO",0,configMDJ) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Punto de montaje: %s \0", (char*)getConfigR("PUNTO_MONTAJE",0,configMDJ) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);


}

///GESTION DE CONEXIONES///
void gestionArchivos(int socketDAM,int operacion){
	char path[50];
	u_int32_t respuesta;
	myRecibirDatosFijos(socketDAM,(char*)path,50);

	switch(operacion){
		case(1):
			printf(BLUE "Validando si existe el archivo '%s'" ,path);
			loading(1);
			if(validarArchivo(path)==0){
				myPuts(BOLDGREEN"Archivo existente" COLOR_RESET "\n");
				respuesta=htonl(0);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			else{
				myPuts(RED "Archivo inexistente" COLOR_RESET "\n");
				respuesta=htonl(1);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			break;
		case(2):
			printf(BLUE "Creando archivo '%s'" ,path);
			loading(1);
			if(crearArchivo(path)==0){
				myPuts(GREEN"Archivo creado" COLOR_RESET "\n");
				respuesta=htonl(0);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			else{
				myPuts(RED "No se pudo crear el archivo" COLOR_RESET "\n");
				respuesta=htonl(1);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			break;
	}

}

void gestionDatos(int socketDAM, int operacion){
	char path[50];
	char datosDummy[30];
	u_int32_t offset,size,respuesta;
	offset=size=respuesta=0;

	myRecibirDatosFijos(socketDAM,(char*)path,50);

	switch(operacion){
		case(3):
			printf(BLUE "Validando si existe el archivo '%s'" ,path);
			loading(1);
			if(validarArchivo(path)==0){
				myPuts(BOLDGREEN"Archivo existente" COLOR_RESET "\n");
				respuesta=htonl(0);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t)); //Le indico al DAM que el archivo existe para que siga operando

				myRecibirDatosFijos(socketDAM,(u_int32_t*)&offset,sizeof(u_int32_t)); //Recibo el offset
				myRecibirDatosFijos(socketDAM,(u_int32_t*)&size,sizeof(u_int32_t)); //Recibo el size
				myPuts(GREEN "Offset: %d\nSize: %d" COLOR_RESET "\n",ntohl(offset),ntohl(size));

				memcpy(datosDummy,"Datos de prueba",16);
				myEnviarDatosFijos(socketDAM,(char*)datosDummy,sizeof(datosDummy)); //TODO Enviar bien los datos *Envio los datos pedidos
			}
			else{
				myPuts(RED "Archivo inexistente" COLOR_RESET "\n");
				respuesta=htonl(1);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			break;
		case(4):
			printf(BLUE "Validando si existe el archivo '%s'" ,path);
			loading(1);
			if(validarArchivo(path)==0){
				myPuts(BOLDGREEN"Archivo existente" COLOR_RESET "\n");
				respuesta=htonl(0);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t)); //Le indico al DAM que el archivo existe para que siga operando

				myRecibirDatosFijos(socketDAM,(u_int32_t*)&offset,sizeof(u_int32_t)); //Recibo el offset
				myRecibirDatosFijos(socketDAM,(u_int32_t*)&size,sizeof(u_int32_t)); //Recibo el size
				myPuts(GREEN "Offset: %d\nSize: %d" COLOR_RESET "\n",ntohl(offset),ntohl(size));
				myRecibirDatosFijos(socketDAM,(char*)datosDummy,sizeof(datosDummy));
				myPuts(GREEN "Datos recibidos: %s" COLOR_RESET "\n");
			}
			else{
				myPuts(RED "Archivo inexistente" COLOR_RESET "\n");
				respuesta=htonl(1);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			break;
	}

}

void gestionarConexionDAM(int sock)
{
	int socketDAM = *(int*)sock;
	u_int32_t buffer=0,operacion=0;
	while(1){

		if(myRecibirDatosFijos(socketDAM,(u_int32_t*)&buffer,sizeof(u_int32_t))==0){
			operacion=ntohl(buffer);

			switch(operacion){
				case(1):
					gestionArchivos(socketDAM,1);
					break;
				case(2):
					gestionArchivos(socketDAM,2);
					break;
				case(3):
					gestionDatos(socketDAM,3);
					break;
				case(4):
					gestionDatos(socketDAM,4);
					break;
			}
		}else{
			myPuts(RED "Se desconecto el proceso DAM" COLOR_RESET "\n");
			break;
		}

	}

}

///FUNCIONES DE CONEXION///

void* connectionDAM()
{
	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t socketDAM; // Descriptor de socket a la escucha
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfigR("IP_ESCUCHA",0,configMDJ));
	PUERTO_ESCUCHA=(int) getConfigR("DAM_PUERTO",1,configMDJ);


	result = myEnlazarServidor((int*) &socketDAM, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos DAM");
		exit(1);
	}

	result = myAtenderClientesEnHilos((int*) &socketDAM, "MDJ", "DAM",(void*) gestionarConexionDAM);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de DAM");
		exit(1);
	}

	return 0;
}

//CONSOLA//

void mkdirr(char* linea,struct tablaDirectory *t_directorios){
		char *pathDirectorioFIFAFS;
		char **split;
		split=(char**)string_split(linea," ");
		pathDirectorioFIFAFS=malloc(strlen(split[1])+1);
		cargarStructDirectorio(t_directorios);
		strcpy(pathDirectorioFIFAFS,split[1]);

		if(crearDirectorio(t_directorios,pathDirectorioFIFAFS)==0)
			actualizarArchivoDirectorio(t_directorios);
		liberarSplit(split);
		free(pathDirectorioFIFAFS);
}

void rm (char* linea,struct tablaDirectory *t_directorios){
	cargarStructDirectorio(t_directorios);
	//char *pathArchivo=string_new();
	char *pathDirectorioFIFAFS=(char*)string_new();
	//char *numeroBloque= string_new();
	//char *numeroCopia= string_new();
	char **split;
	split=(char**)string_split(linea," ");
	if(split[2]!=NULL){
		if(strcmp(split[1],"-d")==0)
		{
			string_append(&pathDirectorioFIFAFS,split[2]);
			if(validarPathDir(pathDirectorioFIFAFS)==0){
				if(borrarDirectorio(t_directorios,pathDirectorioFIFAFS)==0)
					actualizarArchivoDirectorio(t_directorios);
			}
		}
		if(strcmp(split[1],"-b")==0)
		{
			/*string_append(&pathArchivo,split[2]);
			string_append(&numeroBloque,split[3]);
			string_append(&numeroCopia,split[4]);
			if(validarArchivoFS(pathArchivo))
				borrarBloque(pathArchivo,numeroBloque,numeroCopia);
			else
				printf("El archivo especificado no existe\n");*/
			printf("Error: borrado de bloques no implementado");
		}
	}else{
		printf("Error: el comando rm debe ser usado con el parametro -d para borrar un directorio. \n");
	}

	free(pathDirectorioFIFAFS);
	liberarSplit(split);
}

void listarDirectorioIndice(char* linea,struct tablaDirectory *t_directorios){
	char num[2];
	char **split;
	cargarStructDirectorio(t_directorios);
	split=(char**)string_split(linea," ");
	strcpy(num,split[1]);
	printf("Indice directorio: %d\n",t_directorios[atoi(num)].index);
	printf("Nombre directorio: %s\n",t_directorios[atoi(num)].nombre);
	printf("Padre directorio: %d\n",t_directorios[atoi(num)].padre);
	liberarSplit(split);
}

void consola(){
	char* linea;
	tableDirectory t_directorios[100];
	while(1){
		linea = readline(">");
		if (linea)
			add_history(linea);

		if(!strncmp(linea,"config",6))
		{
			mostrarConfig();
		}
		if(!strncmp(linea,"exit",4))
		{
			break;
		}
	 	if(!strncmp(linea,"mkdir",5)){
	 		mkdirr(linea,t_directorios);
	 	}
	 	if(!strncmp(linea,"ls",3)){
	 		printf("Listado de directorios:\n");
	 		cargarStructDirectorio(t_directorios);
			listarDirectorios(t_directorios,0,0);
	 	}
	 	if(!strncmp(linea,"directorio",1)){
			listarDirectorioIndice(linea,t_directorios);
		}
		if(!strncmp(linea,"rm",2)){
			rm(linea,t_directorios);
		}
		free(linea);
	}
}
	///MAIN///

int main(void) {
	system("clear");
	pthread_t hiloConnectionDAM; //Nombre de Hilo a crear
	configMDJ=config_create(PATHCONFIGMDJ);

	pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);
	crearMetadata();
	consola();
	config_destroy(configMDJ); //No llega acá porque se queda en el while(1) de la consola
	return EXIT_SUCCESS;

}
