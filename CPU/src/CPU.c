#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <console/myConsole.h>
#include <conexiones/mySockets.h>

void mostrarConfig(){

    char* myText = string_from_format("DAM   -> IP: %s - Puerto: %s\0",(char*)getConfig("DAM_IP","CPU.txt",0), (char*)getConfig("DAM_PUERTO","CPU.txt",0) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("S-AFA -> IP: %s - Puerto: %s\0", (char*)getConfig("S-AFA_IP","CPU.txt",0), (char*)getConfig("S-AFA_PUERTO","CPU.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Retardo: %s\0" , (char*)getConfig("RETARDO","CPU.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

void* connectionDAM(){
	u_int32_t result,sock;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfig("DAM_IP","CPU.txt",0));
	PUERTO_ESCUCHA=(int)getConfig("DAM_PUERTO","CPU.txt",1);

	result=myEnlazarCliente((int*)&sock,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el DAM para conectarse.\n");
		exit(1);
	}
	return 0;
}

void* connectionSAFA(){
	u_int32_t result,sock;

	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfig("S-AFA_IP","CPU.txt",0));
	PUERTO_ESCUCHA=(int)getConfig("S-AFA_PUERTO","CPU.txt",1);

	result=myEnlazarCliente((int*)&sock,IP_ESCUCHA,PUERTO_ESCUCHA);
	if(result==1){
		myPuts("No se encuentra disponible el S-AFA para conectarse.\n");
		exit(1);
	}
	return 0;
}

int main() {
	pthread_t hiloConnectionDAM;
	pthread_t hiloConnectionSAFA;

	mostrarConfig();

    pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);
    pthread_create(&hiloConnectionSAFA,NULL,(void*)&connectionSAFA,NULL);

    while(1)
    {

    }

	return EXIT_SUCCESS;
}
