#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <console/myConsole.h>
#include <conexiones/mySockets.h>
#include <sentencias/sentencias.h>

u_int32_t socketGFM9;
u_int32_t socketGMDJ;
u_int32_t socketGSAFA;

int maxTransfer;

#define PATHCONFIGDAM "/home/utnso/tp-2018-2c-smlc/Config/DAM.txt"
t_config *configDAM;

/*typedef struct datosProceso {
	char nombreServidor[50];
	char nombreCliente[50];
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;
} thDatos;*/
	///INTERFAZ CON MDJ///

void validarArchivo(char* path){

	u_int32_t respuesta=0;
	u_int32_t operacion = htonl(1);

	myPuts(BLUE "Validando archivo '%s' ",path);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&operacion,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,path,50);
	loading(1);
	myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));

	if(ntohl(respuesta)==0){
		myPuts(BOLDGREEN"El archivo especificado existe" COLOR_RESET "\n");
	}else{
		myPuts(RED "El archivo especificado no existe" COLOR_RESET "\n");
	}

}

int  borrarArchivo(char* path){

	u_int32_t respuesta=0;
	u_int32_t operacion = htonl(6);

	myPuts(BLUE "Borrando '%s' ",path);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&operacion,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,path,50);
	loading(1);
	myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));

	if(ntohl(respuesta)==0){
		myPuts(BOLDGREEN"El archivo fue borrado correctamente" COLOR_RESET "\n");
	}else{
		myPuts(RED "El archivo no pudo ser borrado" COLOR_RESET "\n");
	}

	return respuesta;
}

int crearArchivo(char* path,u_int32_t size){

	u_int32_t respuesta=0;
	u_int32_t operacion = htonl(2);
	u_int32_t sizeFile = htonl(size);

	myPuts(BLUE "Creando '%s' ",path);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&operacion,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,path,50);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&sizeFile,sizeof(u_int32_t));
	loading(1);
	myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));

	if(ntohl(respuesta)==0){
		myPuts(BOLDGREEN"El archivo fue creado correctamente" COLOR_RESET "\n");

	}else{
		myPuts(RED "El archivo no pudo ser creado" COLOR_RESET "\n");
	}

	return respuesta;
}

char* obtenerDatos(char* path,u_int32_t offset, u_int32_t size){

	u_int32_t respuesta=0;
	u_int32_t tamDatos=0;
	u_int32_t operacion;
	u_int32_t tempOffset = htonl(offset);
	u_int32_t tempSize = htonl(size);
	u_int32_t pathSize=htonl(strlen(path));
	if(size==0)
		operacion=htonl(5);
	else
		operacion=htonl(3);

	myPuts(BLUE "Obteniendo datos '%s' ",path);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&operacion,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&pathSize,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,path,ntohl(pathSize)+1);
	loading(1);
	myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));

	if(ntohl(respuesta)==0){
		if(size==0){
			myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&tamDatos,sizeof(u_int32_t));
			char* buffer= malloc(ntohl(tamDatos)+1);
			myRecibirDatosFijos(socketGMDJ,(char*)buffer,ntohl(tamDatos));
			memset(buffer+ntohl(tamDatos),'\0',1);

			return buffer;
		}else{//TODO obtener datos con offset y size
			myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&tempOffset,sizeof(u_int32_t));
			myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&tempSize,sizeof(u_int32_t));
			return "ERROR";
		}
	}else{
		myPuts(RED "El archivo solicitado no existe" COLOR_RESET "\n");
		return "ERROR";
	}

}

void enviarDatosFM9(char* datos,u_int32_t tamDatos){
	myPuts(BLUE "Enviando script a FM9 " COLOR_RESET);
	loading(1);
	u_int32_t operacion= htonl(1);
	myEnviarDatosFijos(socketGFM9,(u_int32_t*)&operacion,sizeof(u_int32_t));
	u_int32_t size=htonl(tamDatos);
	myEnviarDatosFijos(socketGFM9,(u_int32_t*)&size,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGFM9,(char*)datos,tamDatos);
}

void guardarDatos(char* path,u_int32_t offset, u_int32_t size,char* buffer){

	u_int32_t respuesta=0;
	char datosDummy[30];
	u_int32_t operacion= htonl(4);
	u_int32_t tempOffset = htonl(offset);
	u_int32_t tempSize = htonl(size);

	myPuts(BLUE "Guardando datos '%s' ",path);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&operacion,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,path,50);
	loading(1);
	myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));

	if(ntohl(respuesta)==0){
		myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&tempOffset,sizeof(u_int32_t));
		myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&tempSize,sizeof(u_int32_t));

		//memcpy(datosDummy,buffer,strlen(buffer));
		strcpy(datosDummy,buffer);
		myEnviarDatosFijos(socketGMDJ,(char*)datosDummy,sizeof(datosDummy));//TODO Hacer que envie bien los datos (Indicar al MDJ los bytes a enviar para luego enviarlos)
		myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));
		if(ntohl(respuesta)==0)
			myPuts(BOLDGREEN"El archivo fue guardado correctamente" COLOR_RESET "\n");
	}else{
		myPuts(RED "El archivo solicitado no existe" COLOR_RESET "\n");
	}

}
///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("CPU   -> IP: %s - Puerto: %s \0", getConfigR("IP_ESCUCHA",0,configDAM), getConfigR("CPU_PUERTO",0,configDAM) );
	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("S-AFA -> IP: %s - Puerto: %s \0", getConfigR("S-AFA_IP",0,configDAM), getConfigR("S-AFA_PUERTO",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("FM9   -> IP: %s - Puerto: %s \0", getConfigR("FM9_IP",0,configDAM), getConfigR("FM9_PUERTO",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("MDJ   -> IP: %s - Puerto: %s \0", getConfigR("MDJ_IP",0,configDAM), getConfigR("MDJ_PUERTO",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("Transfer size: %s\0" COLOR_RESET , getConfigR("TSIZE",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

void enviarAccionASAFA(int accion, int idDTB,int tamanio,char* pathArchivo){

	myEnviarDatosFijos(socketGSAFA,&accion,sizeof(int));
	myEnviarDatosFijos(socketGSAFA,&idDTB,sizeof(int));

	if(accion == ACC_CREAR_OK || accion == ACC_BORRAR_OK ){
		myEnviarDatosFijos(socketGSAFA,&tamanio,sizeof(int));
		myEnviarDatosFijos(socketGSAFA,pathArchivo,tamanio);
	}

	if(accion == ACC_DUMMY_OK || accion == ACC_ABRIR_OK){
		myEnviarDatosFijos(socketGSAFA,&tamanio,sizeof(int)); //tamanio = fileID reutilizo el parametro
	}

}

int contadorLineas(char* texto){
	int lineas=0;

	for(int i=0;i<strlen(texto);i++){
		if(texto[i]=='\n'){
			lineas++;
		}
	}
	return lineas;
}

char** bytesToLineas(char* bytes){//Covierte los bytes a lineas y los devuelve en un array de strings
	char** lineas=malloc(strlen(bytes)+1);
	char** splitLineas=string_split(bytes,"\n");
	memset(lineas,'\0',strlen(bytes)+1);
	for(int i=0;i<contadorLineas(bytes);i++){
		char* linea;
		if(splitLineas[i]!=NULL)
			linea=string_from_format("%s\n",splitLineas[i]);
		else
			linea=string_from_format("\n");
		lineas[i]=linea;
		//printf("%s",linea);
	}
	liberarSplit(splitLineas);
	return lineas;
}

void operacionAlMDJ(int operacion, int socketCPU){
	char * pathArchivo = NULL;
	char * script;
	int tamanio;
	int respuesta;
	int idDTB;
	int cantLineas;
	int hayEspacio;
	int cantConjuntos;
	char *conjAEnviar;

	myRecibirDatosFijos(socketCPU,&idDTB,sizeof(int));

	if(myRecibirDatosFijos(socketCPU,&tamanio,sizeof(int))==1)
		myPuts(RED"Error al recibir el tamaño del path"COLOR_RESET"\n");

	pathArchivo = malloc(tamanio+1);
	memset(pathArchivo,'\0',tamanio+1);

	if(myRecibirDatosFijos(socketCPU,pathArchivo,tamanio)==1)
		myPuts(RED"Error al recibir el path del Archivo"COLOR_RESET"\n");

	myPuts("El path que se recibio es: %s\n",pathArchivo);

	myPuts("Enviando solicitud de archivo al MDJ \n");

	switch(operacion){

	case OPERACION_CREAR:
		respuesta = crearArchivo(pathArchivo,tamanio);

		if(respuesta == 0){
			enviarAccionASAFA(ACC_CREAR_OK, idDTB, tamanio, pathArchivo);
		}else{
			enviarAccionASAFA(ACC_CREAR_ERROR, idDTB, tamanio, pathArchivo);
		}

	break;

	case OPERACION_BORRAR:
		respuesta = borrarArchivo(pathArchivo);

		if(respuesta == 0){
			enviarAccionASAFA(ACC_BORRAR_OK, idDTB, tamanio, pathArchivo);
		}else{
			enviarAccionASAFA(ACC_BORRAR_ERROR, idDTB, tamanio, pathArchivo);
		}

	break;

	}
}

void operacionFlush(int operacion, int socketCPU){
	int tamanio;
	int fileID;
	int idDTB;
	int cantConjuntos;
	char * pathArchivo = NULL;

	myRecibirDatosFijos(socketCPU,&idDTB,sizeof(int));

	myRecibirDatosFijos(socketCPU,&fileID,sizeof(int));

	if(myRecibirDatosFijos(socketCPU,&tamanio,sizeof(int))==1)
		myPuts(RED"Error al recibir el tamaño del path"COLOR_RESET"\n");

	pathArchivo = malloc(tamanio+1);
	memset(pathArchivo,'\0',tamanio+1);

	if(myRecibirDatosFijos(socketCPU,pathArchivo,tamanio)==1)
		myPuts(RED"Error al recibir el recurso"COLOR_RESET"\n");

	myEnviarDatosFijos(socketGFM9,&operacion,sizeof(int));

	myEnviarDatosFijos(socketGFM9,&fileID,sizeof(int));

	myRecibirDatosFijos(socketCPU,&cantConjuntos,sizeof(int)); //TODO ¿?

	for(int i=0; i < cantConjuntos; i++){
		;//Enviar a MDJ
	}
}

int tamSplit(char** split){
	int i=0;
	while(split[i]){
		i++;
	}
	return i;
}


void operacionDummyOAbrir(int operacion, int socketCPU){
	char * escriptorio;
	char** lineasScript;
	int largoRuta;
	int idDTB;
	int cantLineas;
	myRecibirDatosFijos(socketCPU,&idDTB,sizeof(int));

	myRecibirDatosFijos(socketCPU,&largoRuta,sizeof(int));

	escriptorio = malloc(largoRuta+1);
	memset(escriptorio,'\0',largoRuta+1);

	myRecibirDatosFijos(socketCPU,escriptorio,largoRuta);

	char * script = obtenerDatos(escriptorio,0,0);
	printf("%s",script);

//	myEnviarDatosFijos(socketGFM9,&operacion,sizeof(int));
//
//	cantLineas = contadorLineas(script);
//	myEnviarDatosFijos(socketGFM9,&cantLineas,sizeof(int));

	int hayEspacio=0; // 0 hay 1 no hay espacio
//	myRecibirDatosFijos(socketGFM9,&hayEspacio,sizeof(int));

	if (hayEspacio == 0){
//
//		int cantConjuntos = (strlen(script) / maxTransfer)+1;
//		myEnviarDatosFijos(socketGFM9,&cantConjuntos,sizeof(int));
//
//		char *conjAEnviar = malloc(maxTransfer+1);
//
//		for(int i=0; i<cantConjuntos;i++){
//			memset(conjAEnviar,'\0',maxTransfer+1);
//			strcpy(conjAEnviar,script[i*maxTransfer]);
//			myEnviarDatosFijos(socketGFM9,&conjAEnviar,maxTransfer);
//		}

		int fileID = -1;
		//myRecibirDatosFijos(socketGFM9,&fileID,sizeof(int));

		if(operacion == OPERACION_DUMMY){
			enviarAccionASAFA(ACC_DUMMY_OK,idDTB,fileID,NULL);
		} else {
			enviarAccionASAFA(ACC_ABRIR_OK,idDTB,fileID,NULL);
		}

	} else {
		if(operacion == OPERACION_DUMMY){
			enviarAccionASAFA(ACC_DUMMY_ERROR,idDTB,0,NULL);
		} else {
			enviarAccionASAFA(ACC_ABRIR_ERROR,idDTB,0,NULL);
		}

	}

	/*lineasScript = bytesToLineas(script);//Guarda un array de lineas en lineasScript, cada posicion del array es una linea del script

	while(lineasScript[i]!=NULL){//Print de prueba
		printf("%s",lineasScript[i]);
		i++;
	}*/


	//tamScript=contadorLineas(script);//Tamaño del script en lineas

	//myEnviarDatosFijos(socketGFM9,&tamScript,sizeof(int));
	//TODO ver si se envia todo el script o si hay que dividirlo en lineas

	//liberarSplit(lineasScript);
	free(script);
	free(escriptorio);

}

	///GESTION DE CONEXIONES///

void avisarDesconexionCPU(int socketCPU){
	int accion;

	accion = DESCONEXION_CPU;
	myEnviarDatosFijos(socketGSAFA,&accion,sizeof(int)); 	  //Le aviso que se desconecto una CPU

}

void gestionarConexionCPU(int *sock_cliente){
	int socketCPU = *(int*)sock_cliente;
	int operacion;
	int result;

	while(1){
		result = myRecibirDatosFijos(socketCPU,&operacion,sizeof(int));

		if(result != 1 ){
			switch(operacion){

			case OPERACION_DUMMY:

				operacionDummyOAbrir(operacion, socketCPU);

			break;

			case OPERACION_ABRIR:

				operacionDummyOAbrir(operacion, socketCPU);

			break;

			case OPERACION_FLUSH: //AL FM9

				operacionFlush(operacion, socketCPU);

			break;

			case OPERACION_CREAR:

				operacionAlMDJ(operacion,socketCPU);

			break;

			case OPERACION_BORRAR:

				operacionAlMDJ(operacion,socketCPU);

			break;
			}
		}else{

			avisarDesconexionCPU(socketCPU);

			myPuts(RED "Se desconecto la CPU NRO: %d"COLOR_RESET"\n ", socketCPU);

			break;
		}
	}
}

void gestionarConexionSAFA(int socketSAFA){

	socketGSAFA = socketSAFA;

	int cantidadDeArchivos,fileID;
	int operacion=OPERACION_CLOSE;

	while(1){
		if(myRecibirDatosFijos(socketGSAFA,&cantidadDeArchivos,sizeof(int))!=1){

			myEnviarDatosFijos(socketGFM9,&operacion,sizeof(int));
			myEnviarDatosFijos(socketGFM9,&cantidadDeArchivos,sizeof(int));

			for(int i = 0; i < cantidadDeArchivos; i++){
				if(myRecibirDatosFijos(socketGSAFA,&fileID,sizeof(int))==1){
					myPuts(RED "Error al recibir el fileID del archivo en la posicion nro  %d"COLOR_RESET"\n",i);
				}else{
					myEnviarDatosFijos(socketGFM9,&fileID,sizeof(int));
				}
			}

		}else{
			myPuts(RED "Se desconecto el proceso S-AFA"COLOR_RESET"\n ");

			exit(1);
		}
	}
}

void gestionarConexionFM9(){

	maxTransfer = (int)getConfigR("TSIZE",1,configDAM);

	myEnviarDatosFijos(socketGFM9,&maxTransfer,sizeof(int));

}

void gestionarConexionMDJ(){

	//validarArchivo("../root/bou.txt");
	//crearArchivo("scripts/prueba.archivo",256);
	//borrarArchivo("scripts/prueba.archivo");
	//char *archivo=obtenerDatos("scripts/checkpoint.escriptorio",0,0);
	//enviarDatosFM9(archivo,strlen(archivo));
	//free(archivo);
	//guardarDatos("root/fifa/jugadores/bou.txt",40,100,"Numero->9");
	//obtenerScript("scripts/checkpoint.escriptorio");
}

	///FUNCIONES DE CONEXION///

void *connectionCPU() {

	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t servidor; // Descriptor de socket a la escucha
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfigR("IP_ESCUCHA",0,configDAM));
	PUERTO_ESCUCHA=(int) getConfigR("CPU_PUERTO",1,configDAM);

	result = myEnlazarServidor((int*) &servidor, &direccionServidor,IP_ESCUCHA, PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos CPU");
		exit(1);
	}

	myAtenderClientesEnHilos((int*) &servidor, "DAM", "CPU", gestionarConexionCPU);

	if (result != 0) {
		myPuts("No fue posible atender requerimientos de CPU");
		exit(1);
	}

	return 0;
}

void *connectionSAFA(){
	u_int32_t result,socketSAFA;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfigR("S-AFA_IP",0,configDAM));
	PUERTO_ESCUCHA=(int)getConfigR("S-AFA_PUERTO",1,configDAM);

	result=myEnlazarCliente((int*)&socketSAFA,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el S-AFA para conectarse.\n");
		exit(1);
	}

	gestionarConexionSAFA(socketSAFA);
	return 0;
}

void *connectionFM9(){
	u_int32_t result;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfigR("FM9_IP",0,configDAM));
	PUERTO_ESCUCHA=(int)getConfigR("FM9_PUERTO",1,configDAM);

	result=myEnlazarCliente((int*)&socketGFM9,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el FM9 para conectarse.\n");
		exit(1);
	}
	gestionarConexionFM9();
	return 0;
}

void *connectionMDJ(){
	u_int32_t result;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfigR("MDJ_IP",0,configDAM));
	PUERTO_ESCUCHA=(int)getConfigR("MDJ_PUERTO",1,configDAM);

	result=myEnlazarCliente((int*)&socketGMDJ,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el MDJ para conectarse.\n");
		exit(1);
	}

	gestionarConexionMDJ();
	return 0;
}

	///MAIN///

int main() {
	system("clear");
	pthread_t hiloConnectionCPU;
	pthread_t hiloConnectionSAFA;
	pthread_t hiloConnectionFM9;
	pthread_t hiloConnectionMDJ;

	configDAM=config_create(PATHCONFIGDAM);


    pthread_create(&hiloConnectionSAFA,NULL,(void*)&connectionSAFA,NULL);
	pthread_create(&hiloConnectionMDJ,NULL,(void*)&connectionMDJ,NULL);
    //pthread_create(&hiloConnectionFM9,NULL,(void*)&connectionFM9,NULL);
    pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);

    mostrarConfig();
    while(1)
    {

    }

	return EXIT_SUCCESS;
}
