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

void gestionarConexionDAM(int sock)
{
	int socketDAM = *(int*)sock;
	while(1){
		if(gestionarDesconexion((int)socketDAM,"DAM")!=0)
			break;
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
		free(split[0]);
		free(split[1]);
		free(split);
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
	free(split[0]);
	free(split[1]);
	free(split[2]);
	free(split);
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
	free(split);
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
