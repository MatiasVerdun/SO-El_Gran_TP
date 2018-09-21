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
#include <dtbSerializacion/dtbSerializacion.h>

#define PATHCONFIGCPU "/home/utnso/tp-2018-2c-smlc/Config/CPU.txt"
t_config *configCPU;

	///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("DAM   -> IP: %s - Puerto: %s\0",(char*)getConfigR("DAM_IP",0,configCPU), (char*)getConfigR("DAM_PUERTO",0,configCPU) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("S-AFA -> IP: %s - Puerto: %s\0", (char*)getConfigR("S-AFA_IP",0,configCPU), (char*)getConfigR("S-AFA_PUERTO",0,configCPU) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Retardo: %s\0" , (char*)getConfigR("RETARDO",0,configCPU) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

	///GESTION DE CONEXIONES///

void gestionarConexionSAFA(int socketSAFA){

	recibirDTB(socketSAFA);

	/*while(1){
		if(gestionarDesconexion((int)socketSAFA,"SAFA")!=0)
			break;
	}*/

	/*//A modo de prueba solo para probar el envio de mensajes entre procesos, no tiene ninguna utilidad
	char buffer[5];
	myRecibirDatosFijos(socketSAFA,buffer,5);
	printf("El buffer que recibi por socket es %s\n",buffer);*/
}

void gestionarConexionDAM(int socketDAM){
	while(1){
		if(gestionarDesconexion((int)socketDAM,"DAM")!=0)
			break;
	}
}

void gestionarConexionFM9(int socketFM9){
	while(1){
		if(gestionarDesconexion((int)socketFM9,"FM9")!=0)
			break;
	}
}
	///FUNCIONES DE CONEXION///

void* connectionFM9(){
	u_int32_t result,socketFM9;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfigR("FM9_IP",0,configCPU));
	PUERTO_ESCUCHA=(int)getConfigR("FM9_PUERTO",1,configCPU);

	result=myEnlazarCliente((int*)&socketFM9,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el DAM para conectarse.\n");
		exit(1);
	}

	gestionarConexionFM9(socketFM9);
	return 0;
}

void* connectionDAM(){
	u_int32_t result,socketDAM;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfigR("DAM_IP",0,configCPU));
	PUERTO_ESCUCHA=(int)getConfigR("DAM_PUERTO",1,configCPU);

	result=myEnlazarCliente((int*)&socketDAM,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el DAM para conectarse.\n");
		exit(1);
	}

	gestionarConexionDAM(socketDAM);
	return 0;
}

void* connectionSAFA(){
	u_int32_t result,socketSAFA;

	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfigR("S-AFA_IP",0,configCPU));
	PUERTO_ESCUCHA=(int)getConfigR("S-AFA_PUERTO",1,configCPU);

	result=myEnlazarCliente((int*)&socketSAFA,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el S-AFA para conectarse.\n");
		exit(1);
	}

	gestionarConexionSAFA((int)socketSAFA);
	return 0;
}

	///MAIN///

int main() {
	system("clear");
	pthread_t hiloConnectionDAM;
	pthread_t hiloConnectionSAFA;
	pthread_t hiloConnectionFM9;

	configCPU=config_create(PATHCONFIGCPU);

	mostrarConfig();

    //pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);
    pthread_create(&hiloConnectionSAFA,NULL,(void*)&connectionSAFA,NULL);
    //pthread_create(&hiloConnectionFM9,NULL,(void*)&connectionFM9,NULL);

    while(1)
    {

    }

	return EXIT_SUCCESS;
}
