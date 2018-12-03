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


static sem_t semDAM;


void inicializarSemaforos(){
	sem_init(&semDAM,0,1);
}

///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("DAM -> IP: %s - Puerto: %s \0", (char*)getConfigR("IP_ESCUCHA",0,configMDJ),(char*)getConfigR("DAM_PUERTO",0,configMDJ) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Punto de montaje: %s \0", (char*)getConfigR("PUNTO_MONTAJE",0,configMDJ) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Retardo: %s \0", (char*)getConfigR("RETARDO",0,configMDJ) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

///GESTION DE CONEXIONES///
void gestionArchivos(int socketDAM,int operacion){
	u_int32_t respuesta,size,pathSize;
	char *path,*pathProvisorio;

	myRecibirDatosFijos(socketDAM,(u_int32_t*)&pathSize,sizeof(u_int32_t));
	pathProvisorio=malloc(ntohl(pathSize)+1);
	myRecibirDatosFijos(socketDAM,(char*)pathProvisorio,ntohl(pathSize)+1);
	path=string_from_format("Archivos%s",pathProvisorio);

	switch(operacion){
		case(1)://Validar existencia de archivo
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
		case(2)://Crear archivo
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
		case(6)://Borrar archivo
			printf(BLUE "Borrando archivo '%s'" ,path);
			loading(1);
			if(borrarArchivo(path)==0){
				myPuts(BOLDGREEN"Archivo borrardo" COLOR_RESET "\n");
				respuesta=htonl(0);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			else{
				myPuts(RED "No se pudo borrar el archivo" COLOR_RESET "\n");
				respuesta=htonl(1);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			break;
	}
	free(pathProvisorio);
	free(path);
}

void gestionDatos(int socketDAM, int operacion){
	char* path,*pathProvisorio;
	u_int32_t offset,size,respuesta,pathSize,transferSize;
	offset=size=respuesta=0;
	myRecibirDatosFijos(socketDAM,(u_int32_t*)&transferSize,sizeof(u_int32_t));
	myRecibirDatosFijos(socketDAM,(u_int32_t*)&pathSize,sizeof(u_int32_t));
	//printf("%d\n",ntohl(pathSize));
	transferSize=ntohl(transferSize);
	pathProvisorio=malloc(ntohl(pathSize)+1);
	myRecibirDatosFijos(socketDAM,(char*)pathProvisorio,ntohl(pathSize)+1);
	path=string_from_format("Archivos%s",pathProvisorio);
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
				char* datos=obtenerDatos(path,ntohl(offset),ntohl(size));
				enviarDatosTS(socketDAM,datos,transferSize);
				free(datos);
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
				char* datos=recibirDatosTS(socketDAM,transferSize);
				if(guardarDatos(path,ntohl(offset),ntohl(size),datos)==0){
					myPuts(GREEN "Se guardaron los datos con exito" COLOR_RESET "\n");
				}else{
					myPuts(GREEN "No se pudieron guardar los datos" COLOR_RESET "\n");
					respuesta=htonl(1);
				}

				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
				free(datos);
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
				enviarDatosTS(socketDAM,archivo,transferSize);
				/*myEnviarDatosFijos(socketDAM,(u_int32_t*)&size,sizeof(u_int32_t));
				myEnviarDatosFijos(socketDAM,(char*)archivo,ntohl(size));*/
				free(archivo);
			}
			else{
				myPuts(RED "Archivo inexistente" COLOR_RESET "\n");
				respuesta=htonl(1);
				myEnviarDatosFijos(socketDAM,(u_int32_t*)&respuesta,sizeof(u_int32_t));
			}
			break;
	}
	free(pathProvisorio);
	free(path);
}

void gestionarConexionDAM(int sock)
{
	int socketDAM = *(int*)sock;
	u_int32_t buffer=0,operacion=0;
	while(1){

		if(myRecibirDatosFijos(socketDAM,(u_int32_t*)&buffer,sizeof(u_int32_t))==0){
			sem_wait(&semDAM);
			usleep((int)getConfigR("RETARDO",1,configMDJ));
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
				case(6):
					gestionArchivos(socketDAM,6);
					break;
			}
			sem_post(&semDAM);
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

void listar(char* linea){
    struct dirent *de;  // Pointer for directory entry
	char* realpath;
	char* puntoMontaje=(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ);
	int contadorElementos=0;
	if(strcmp(linea,"")==0){
		printf(BOLDCYAN "fifafs:~/mnt/%s" COLOR_RESET "\n",dirActual+strlen(puntoMontaje));
		realpath=string_from_format("%s",dirActual);
	}else{
		char** split=string_split(linea," ");
		if(split[1]!=NULL){
			printf(BOLDCYAN "fifafs:~/mnt/%s" COLOR_RESET "\n",split[1]);
			realpath=string_from_format("%s/%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ),split[1]);
		}else{
			printf(BOLDCYAN "fifafs:~/mnt/%s" COLOR_RESET "\n",dirActual+strlen(puntoMontaje));
			realpath=string_from_format("%s",dirActual);
		}
	    liberarSplit(split);
	}
	if(!existeCarpeta(realpath)){
		printf("La ruta especificada es invalida\n");
		return;
	}
	DIR *dr = opendir(realpath);
	if (dr == NULL)
		printf("La ruta especificada es invalida\n");


	while ((de = readdir(dr)) != NULL){
		if(esArchivo(de->d_name)==0){
			if(strcmp(de->d_name,".")!=0 && strcmp(de->d_name,"..")!=0)
				printf(BOLDMAGENTA "%s\n" COLOR_RESET, de->d_name);
		}
		else
			printf(BOLDBLUE "%s\n" COLOR_RESET, de->d_name);
	}

	closedir(dr);
    free(realpath);
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

void cd(char* linea){
	char **split;
	split=(char**)string_split(linea," ");
	char* pathPedido;
	char* puntoMontaje=(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ);
	if(strcmp(split[1],".")==0){
		listar("/");
		//printf(BOLDCYAN "fifafs:~mnt/%s" COLOR_RESET "\n",dirActual+strlen(puntoMontaje));
	}else{
		if(strcmp(split[1],"..")==0){
			if(strcmp(dirActual,puntoMontaje)!=0){
				pathPedido=obtenerDirAnterior(dirActual);
			}else{
				listar("/");
				liberarSplit(split);
				return;
			}
		}else
			pathPedido=string_from_format("%s%s/",dirActual,split[1]);

		//printf("%s\n",aux);
		if(existeCarpeta(pathPedido)==1){
			free(dirActual);
			dirActual=pathPedido;
			listar(dirActual+strlen(puntoMontaje));
		}else{
			free(pathPedido);
			printf("El directorio especificado no existe\n");
		}
	}
	liberarSplit(split);
}

void cat(char* linea){
	char *pathArchivoFIFAFS,*archivo;
	char **split;
	split=(char**)string_split(linea," ");
	char* puntoMontaje=(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ);
	if(esRutaFS(split[1])==0){
		pathArchivoFIFAFS=string_from_format("%s",split[1]);
	}else{
		pathArchivoFIFAFS=string_from_format("%s%s",dirActual+strlen(puntoMontaje),split[1]);
	}
	if(esArchivoFS(pathArchivoFIFAFS)==1){
		free(pathArchivoFIFAFS);
		liberarSplit(split);
		printf("El archivo a leer no es un archivo del FIFAFS\n");
		return;
	}
	archivo=obtenerArchivoFS(pathArchivoFIFAFS);
	if(strcmp(archivo,"ERROR")==0){
		printf("El archivo especificado no existe\n");
	}else{
		printf("%s",archivo);
		free(archivo);
	}
	free(pathArchivoFIFAFS);
	liberarSplit(split);

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
		add_history("mkfile Archivos/scripts/creacion.escriptorio 50");
		//add_history("cat Archivos/scripts/checkpoint.escriptorio");
		add_history("cat Archivos/scripts/creacion.escriptorio");
		add_history("getData Archivos/scripts/creacion.escriptorio 25 25");
		add_history("rmfile equipos/equipo1.txt");
		add_history("mkfile /jugadores/Bou.txt");

		if (linea)
			add_history(linea);

		if(!strncmp(linea,"config",6))
		{
			mostrarConfig();
		}
	 	if(!strncmp(linea,"ls",2)){
	 		listar(linea);
	 	}
		if(!strncmp(linea,"fs",2)){
			cargarFS();
		}
		if(!strncmp(linea,"cat",2)){
			cat(linea);
		}
		if(!strncmp(linea,"cd",2)){
			cd(linea);
		}
		if(!strncmp(linea,"mkfile",6)){
			char** split=string_split(linea," ");
			crearArchivo(split[1],atoi(split[2]));
			liberarSplit(split);
		}
		if(!strncmp(linea,"rmfile",6)){
			char** split=string_split(linea," ");
			borrarArchivo(split[1]);
			liberarSplit(split);
		}
		if(!strncmp(linea,"bm",2)){
			mostrarBitmap();
		}
		if(!strncmp(linea,"md5 ",4)){
			char** split;
			char path[256];
			char temp[263];
			split = string_split(linea, " ");

			strcpy(path, split[1]);
			//strcpy(path, "/home/utnso/tp-2018-2c-smlc/Config/CPU.txt");

			strcpy(temp, "md5sum ");
			strcat(temp,path);
			system(temp);

			free(split[0]);
			free(split[1]);
			free(split);
			break;
		}
		if(!strncmp(linea,"getData",7)){
			char* datos;
			char **split;
			split=(char**)string_split(linea," ");
			datos=obtenerDatos(split[1],atoi(split[2]),atoi(split[3]));
			printf("%s\n",datos);
			liberarSplit(split);
			free(datos);
		}
		if(!strncmp(linea,"save",4)){
			char** split=string_split(linea," ");
			char* datos=obtenerArchivoFS("Archivos/scripts/checkpoint.escriptorio");
			int tamDatos=obtenerTamArchivoFS("Archivos/scripts/checkpoint.escriptorio");
			guardarDatos(split[1],0,tamDatos,datos);
			liberarSplit(split);
			free(datos);
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
		if(!strncmp(linea,"exit",4)){
			free(bitmap->bitarray);
			free(bitmap);
			free(linea);
			free(dirActual);
			break;
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
	inicializarSemaforos();
	crearMetadata();
	cargarFS();
	consola();
	config_destroy(configMDJ);
	return EXIT_SUCCESS;

}
