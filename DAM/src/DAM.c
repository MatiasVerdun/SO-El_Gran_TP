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

u_int32_t socketGFM9;
u_int32_t socketGMDJ;

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

void crearArchivo(char* path){

	u_int32_t respuesta=0;
	u_int32_t operacion = htonl(2);

	myPuts(BLUE "Creando '%s' ",path);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&operacion,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,path,50);
	loading(1);
	myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));

	if(ntohl(respuesta)==0){
		myPuts(GREEN"El archivo fue creado correctamente" COLOR_RESET "\n");
	}else{
		myPuts(RED "El archivo no pudo ser creado" COLOR_RESET "\n");
	}

}

void obtenerDatos(char* path,u_int32_t offset, u_int32_t size){

	u_int32_t respuesta=0;
	char *buffer=malloc(size);
	u_int32_t operacion= htonl(3);
	u_int32_t tempOffset = htonl(offset);
	u_int32_t tempSize = htonl(size);

	myPuts(BLUE "Obteniendo datos '%s' ",path);
	myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&operacion,sizeof(u_int32_t));
	myEnviarDatosFijos(socketGMDJ,path,50);
	loading(1);
	myRecibirDatosFijos(socketGMDJ,(u_int32_t*)&respuesta,sizeof(u_int32_t));

	if(ntohl(respuesta)==0){
		myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&tempOffset,sizeof(u_int32_t));
		myEnviarDatosFijos(socketGMDJ,(u_int32_t*)&tempSize,sizeof(u_int32_t));

		myRecibirDatosFijos(socketGMDJ,(char*)buffer,30);//TODO Hacer que reciba bien los datos (Recibir cuantos datos se van a mandar para luego recibirlos)
		memset(buffer+30,'\0',1);
		myPuts("Los datos recibidos son: %s\n",buffer);
	}else{
		myPuts(RED "El archivo solicitado no existe" COLOR_RESET "\n");
	}

	free(buffer);
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
	myText = string_from_format("S-AFA -> IP: %s - Puerto: %s \0", getConfigR("S-AFA_IP",0,configDAM), getConfigR("S-AFA_PUERTO",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("FM9   -> IP: %s - Puerto: %s \0", getConfigR("FM9_IP",0,configDAM), getConfigR("FM9_PUERTO",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("MDJ   -> IP: %s - Puerto: %s \0", getConfigR("MDJ_IP",0,configDAM), getConfigR("MDJ_PUERTO",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Transfer size: %s\0" COLOR_RESET , getConfigR("TSIZE",0,configDAM) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

	///GESTION DE CONEXIONES///

void gestionarConexionCPU(int *sock_cliente){
	int socketCPU = *(int*)sock_cliente;

	char buffer[256];
	char strLenRuta[4];
	int largoRuta;

	myRecibirDatosFijos(socketCPU,buffer,3);

	strncpy(strLenRuta,buffer,3);
	strLenRuta[4] = '\0';
	largoRuta = atoi(strLenRuta);

	myRecibirDatosFijos(socketCPU,buffer,largoRuta);

	myPuts("La ruta del Escriptorio que se recibio es: %s\n",buffer);
}

void gestionarConexionSAFA(int socketSAFA){

}

void gestionarConexionFM9(){

}

void gestionarConexionMDJ(){

	//validarArchivo("root/fifa/jugadores/bou.txt");
	//crearArchivo("root/fifa/jugadores/bou.txt");
	//obtenerDatos("root/fifa/jugadores/bou.txt",40,100);
	//guardarDatos("root/fifa/jugadores/bou.txt",40,100,"Numero->9");
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
    //pthread_create(&hiloConnectionFM9,NULL,(void*)&connectionFM9,NULL);
    //pthread_create(&hiloConnectionMDJ,NULL,(void*)&connectionMDJ,NULL);
    pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);

    mostrarConfig();
    while(1)
    {

    }

	return EXIT_SUCCESS;
}
