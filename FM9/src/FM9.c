#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <console/myConsole.h>
#include <conexiones/mySockets.h>

	///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("DAM -> IP: %s - Puerto: %s ", (char*)getConfig("IP_ESCUCHA","FM9.txt",0), (char*)getConfig("DAM_PUERTO","FM9.txt",0) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Tam. maximo de memoria: %s", (char*)getConfig("TMM","FM9.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Tam. linea: %s\0", (char*)getConfig("TML","FM9.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Tam. de pagina: %s\0", (char*)getConfig("TMP","FM9.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

	///GESTION DE CONEXIONES///

void gestionarConexionDAM(int sock){
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
	u_int32_t servidor; // Descriptor de socket a la escucha
	int socketDAM;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfig("IP_ESCUCHA","FM9.txt",0));
	PUERTO_ESCUCHA=(int) getConfig("DAM_PUERTO","FM9.txt",1);


	result = myEnlazarServidor((int*) &servidor, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos DAM");
		exit(1);
	}

	result = myAtenderClientesEnHilos((int*) &servidor, "FM9", "DAM", gestionarConexionDAM);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de DAM");
		exit(1);
	}

	return 0;
}

	///MAIN///

int main() {
	system("clear");
	pthread_t hiloConnectionDAM;

	mostrarConfig();

    pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);

    while(1)
    {

    }

	return EXIT_SUCCESS;
}




