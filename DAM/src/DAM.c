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

typedef struct datosProceso {
	char nombreServidor[50];
	char nombreCliente[50];
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;
} thDatos;

	///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("CPU   -> IP: %s - Puerto: %s \0", getConfig("IP_ESCUCHA","DAM.txt",0), getConfig("CPU_PUERTO","DAM.txt",0) );
	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("S-AFA -> IP: %s - Puerto: %s \0", getConfig("S-AFA_IP","DAM.txt",0), getConfig("S-AFA_PUERTO","DAM.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("FM9   -> IP: %s - Puerto: %s \0", getConfig("FM9_IP","DAM.txt",0), getConfig("FM9_PUERTO","DAM.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("MDJ   -> IP: %s - Puerto: %s \0", getConfig("MDJ_IP","DAM.txt",0), getConfig("MDJ_PUERTO","DAM.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Transfer size: %s\0" COLOR_RESET , getConfig("TSIZE","DAM.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

	///GESTION DE CONEXIONES///

void gestionarConexionCPU(int *sock_cliente){
	int socketCPU = *(int*)sock_cliente;

}

void gestionarConexionSAFA(int *socketSAFA){

}

void gestionarConexionFM9(){

}

void gestionarConexionMDJ(int *socketMDJ){
	while(1){
		if(gestionarDesconexion((int)socketMDJ,"MDJ")!=0)
			break;
	}
}

	///FUNCIONES DE CONEXION///

void *connectionCPU() {

	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t servidor; // Descriptor de socket a la escucha
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfig("IP_ESCUCHA","DAM.txt",0));
	PUERTO_ESCUCHA=(int) getConfig("CPU_PUERTO","DAM.txt",1);

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

	strcpy(IP_ESCUCHA,(char*)getConfig("S-AFA_IP","DAM.txt",0));
	PUERTO_ESCUCHA=(int)getConfig("S-AFA_PUERTO","DAM.txt",1);

	result=myEnlazarCliente((int*)&socketSAFA,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el S-AFA para conectarse.\n");
		exit(1);
	}

	gestionarConexionSAFA((int*) socketSAFA);
	return 0;
}

void *connectionFM9(){
	u_int32_t result,socketFM9;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfig("FM9_IP","DAM.txt",0));
	PUERTO_ESCUCHA=(int)getConfig("FM9_PUERTO","DAM.txt",1);

	result=myEnlazarCliente((int*)&socketGFM9,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el FM9 para conectarse.\n");
		exit(1);
	}
	gestionarConexionFM9();
	return 0;
}

void *connectionMDJ(){
	u_int32_t result,socketMDJ;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfig("MDJ_IP","DAM.txt",0));
	PUERTO_ESCUCHA=(int)getConfig("MDJ_PUERTO","DAM.txt",1);

	result=myEnlazarCliente((int*)&socketMDJ,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el MDJ para conectarse.\n");
		exit(1);
	}

	gestionarConexionMDJ((int*)socketMDJ);
	return 0;
}

	///MAIN///

int main() {
	system("clear");
	pthread_t hiloConnectionCPU;
	pthread_t hiloConnectionSAFA;
	pthread_t hiloConnectionFM9;
	pthread_t hiloConnectionMDJ;

    pthread_create(&hiloConnectionSAFA,NULL,(void*)&connectionSAFA,NULL);
    pthread_create(&hiloConnectionFM9,NULL,(void*)&connectionFM9,NULL);
    pthread_create(&hiloConnectionMDJ,NULL,(void*)&connectionMDJ,NULL);
    pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);

    mostrarConfig();
    while(1)
    {

    }

	return EXIT_SUCCESS;
}
