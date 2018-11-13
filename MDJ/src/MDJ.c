//#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <conexiones/mySockets.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "filesystemFIFA.h"
#include "interfaz.h"

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
	u_int32_t respuesta,size;
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
			myRecibirDatosFijos(socketDAM,(u_int32_t*)&size,sizeof(u_int32_t));
			printf(BLUE "Creando archivo '%s'" ,path);
			loading(1);
			if(crearArchivo(path,ntohl(size))==0){
				myPuts(BOLDGREEN"Archivo creado" COLOR_RESET "\n");
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
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			else{
				myPuts(RED "Archivo inexistente" COLOR_RESET "\n");
				respuesta=htonl(1);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			break;
		case(5):
			printf(BLUE "Validando si existe el archivo '%s'" ,path);
			loading(1);

			if(validarArchivo(path)==0){
				myPuts(BOLDGREEN"Archivo existente enviando script" COLOR_RESET "\n");
				respuesta=htonl(0);

				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t)); //Le indico al DAM que el archivo existe para que siga operando
				char* archivo=obtenerArchivoFS(path);
				size=htonl(obtenerTamArchivoFS(path));

				myEnviarDatosFijos(socketDAM,(u_int32_t*)&size,sizeof(u_int32_t));
				myEnviarDatosFijos(socketDAM,(char*)archivo,ntohl(size));
				free(archivo);
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
				case(1): //Verificar archivo
					gestionArchivos(socketDAM,1);
					break;
				case(2): //Crear archivo
					gestionArchivos(socketDAM,2);
					break;
				case(3): //Obtener datos
					gestionDatos(socketDAM,3);
					break;
				case(4): //Guardar datos
					gestionDatos(socketDAM,4);
					break;
				case(5): //Obtener archivo completo
					gestionDatos(socketDAM,5);
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

void cat(char* linea){
	char *pathArchivoFIFAFS,*archivo;
	char **split;
	split=(char**)string_split(linea," ");
	pathArchivoFIFAFS=malloc(strlen(split[1])+1);
	strcpy(pathArchivoFIFAFS,split[1]);
	archivo=obtenerArchivoFS(pathArchivoFIFAFS);
	if(strcmp(archivo,"ERROR")==0){
		printf("El archivo especificado no existe\n");
	}else{
		printf("Contenido del archivo:\n%s\n",archivo);
		free(archivo);
	}
	free(pathArchivoFIFAFS);
	liberarSplit(split);

}

void listar(char* linea){
    struct dirent *de;  // Pointer for directory entry
	char **split;
	split=(char**)string_split(linea," ");
	char* realpath;

	if(split[1]){

		if(existeArchivoFS(split[1])==0){
			printf(BOLDCYAN "(%s):" COLOR_RESET "\n",split[1]);
			realpath=string_from_format("%sArchivos/%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ),split[1]);
		}else{
			printf("La ruta especificada es invalida\n");
			return ;
		}

	}else{
		printf(BOLDCYAN "(root):" COLOR_RESET "\n");
		realpath= string_from_format("%sArchivos/",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	}

    DIR *dr = opendir(realpath);
    if (dr == NULL)
        printf("Could not open current directory" );


    while ((de = readdir(dr)) != NULL){
    	if(esArchivo(de->d_name)==0)
    		printf("%s\n", de->d_name);
    	else
    		printf(BOLDBLUE "%s" COLOR_RESET"\n", de->d_name);
    }
    closedir(dr);
    liberarSplit(split);
    free(realpath);;
}

void crearBitmap(char *bitarray,int size){
	t_bitarray *bitmap;
	bitmap = bitarray_create_with_mode(bitarray, 64, MSB_FIRST);
	for(int i=0;i<(bitmap->size);i++){
		printf("%d",bitarray_test_bit(bitmap,i));
	}
	printf("\n");
	free(bitmap);
}

void leerArchivoBitmap(){
	/*unsigned char key[8];
	FILE *secretkey;
	secretkey = fopen("/home/utnso/fifa-examples/fifa-checkpoint/Metadata/Bitmap.bin", "r+b");
	fread(key, 1, sizeof key, secretkey);
	for(int j = 0; j < sizeof(key) ; j++) {
	    printf("%02x ", key[j]);
	}
	printf("\n");*/

	size_t sizeArchivo=tamArchivo("/home/utnso/fifa-examples/fifa-checkpoint/Metadata/Bitmap.bin");

	char bitmap[sizeArchivo];
	FILE *secretkey;
	secretkey = fopen("/home/utnso/fifa-examples/fifa-checkpoint/Metadata/Bitmap.bin", "r+b");
	fread(bitmap, 1, sizeArchivo, secretkey);
	for(int j = 0; j < sizeArchivo ; j++) {
	    printf("%02x ", bitmap[j]);
	}
	printf("\n");
}

void consola(){
	char* linea;
	tableDirectory t_directorios[100];
	while(1){
		linea = readline(">");
		add_history("cat scripts/checkpoint.escriptorio");
		if (linea)
			add_history(linea);

		if(!strncmp(linea,"config",6))
		{
			mostrarConfig();
		}
		if(!strncmp(linea,"exit",4))
		{
			free(bitmap->bitarray);
			free(bitmap);
			free(linea);
			break;
		}
	 	if(!strncmp(linea,"mkdir",5)){
	 		mkdirr(linea,t_directorios);
	 	}
	 	if(!strncmp(linea,"ls",2)){
	 		listar(linea);
	 		/*cargarStructDirectorio(t_directorios);
			listarDirectorios(t_directorios,0,0);*/
	 	}
	 	if(!strncmp(linea,"directorio",1)){
			listarDirectorioIndice(linea,t_directorios);
		}
		if(!strncmp(linea,"rm",2)){
			rm(linea,t_directorios);
		}
		if(!strncmp(linea,"fs",2)){
			cargarFS();
		}
		if(!strncmp(linea,"cat",2)){
			cat(linea);
		}
		if(!strncmp(linea,"mkfile",6)){
			crearArchivo("scripts/checkpoint.escriptorio",201);
		}
		if(!strncmp(linea,"bm",2)){
			mostrarBitmap();
		}
		if(!strncmp(linea,"get",3)){
			printf("Bloque libre: %d\n",(int)getNBloqueLibre()+1);
		}
		if(!strncmp(linea,"pbm",3)){
			char **split;
			split=(char**)string_split(linea," ");
			int tipo= atoi(split[1]);
			pruebaBitmap(tipo);
			liberarSplit(split);

		}
		if(!strncmp(linea,"set",3)){
			char **split;
			split=(char**)string_split(linea," ");
			int indice= atoi(split[1]);
			setBloqueOcupado(indice);
			liberarSplit(split);

		}
		if(!strncmp(linea,"clear",4)){
			char **split;
			split=(char**)string_split(linea," ");
			int indice= atoi(split[1]);
			setBloqueLibre(indice);
			liberarSplit(split);

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
	cargarFS();
	consola();
	config_destroy(configMDJ);
	return EXIT_SUCCESS;

}
