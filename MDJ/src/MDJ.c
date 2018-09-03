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

void mostrarConfig(){

    char* myText = string_from_format("DAM -> IP: %s - Puerto: %s \0", (char*)getConfig("IP_ESCUCHA","MDJ.txt",0),(char*)getConfig("DAM_PUERTO","MDJ.txt",0) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Punto de montaje: %s \0", (char*)getConfig("PUNTO_MONTAJE","MDJ.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);


}

void gestionarConexionDAM()
{

}

void* connectionDAM()
{
	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t servidor; // Descriptor de socket a la escucha
	int sock_Cliente;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfig("IP_ESCUCHA","MDJ.txt",0));
	PUERTO_ESCUCHA=(int) getConfig("DAM_PUERTO","MDJ.txt",1);


	result = myEnlazarServidor((int*) &servidor, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos DAM");
		exit(1);
	}

	result = myAtenderCliente((int*)&servidor, "MDJ", "DAM", &sock_Cliente);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de DAM");
		exit(1);
	}

	gestionarConexionDAM();

	return 0;
}

int main(void) {
	pthread_t hiloConnectionDAM; //Nombre de Hilo a crear

	mostrarConfig();

	pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);

	while(1){

	}
	return EXIT_SUCCESS;
}
