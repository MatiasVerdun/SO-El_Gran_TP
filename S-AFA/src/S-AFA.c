#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <conexiones/mySockets.h>
#include <commons/string.h>
#include <console/myConsole.h>
#include <commons/collections/list.h>

void mostrarConfig(){

    char* myText = string_from_format("DAM -> IP: %s - Puerto: %s \0", (char*)getConfig("IP_ESCUCHA","S-AFA.txt",0),(char*)getConfig("DAM_PUERTO","S-AFA.txt",0) );
	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("CPU -> IP: %s - Puerto: %s \0", (char*)getConfig("IP_ESCUCHA","S-AFA.txt",0),(char*)getConfig("CPU_PUERTO","S-AFA.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Algoritmo: %s \0", (char*)getConfig("ALGO_PLANI","S-AFA.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Grado de multiprogramacion: %s \0", (char*)getConfig("GMP","S-AFA.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Quantum: %s \0", (char*)getConfig("Q","S-AFA.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Retardo: %s \0" COLOR_RESET , (char*)getConfig("RETARDO","S-AFA.txt",0) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

void gestionarConexionCPU()
{

}

void gestionarConexionDAM()
{

}

void* connectionCPU() {

	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t servidor; // Descriptor de socket a la escucha
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfig("IP_ESCUCHA","S-AFA.txt",0));
	PUERTO_ESCUCHA=(int) getConfig("CPU_PUERTO","S-AFA.txt",1);


	result = myEnlazarServidor((int*) &servidor, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos CPU");
		exit(1);
	}

	myAtenderClientesEnHilos((int*) &servidor, "S-AFA", "CPU", gestionarConexionCPU);

	if (result != 0) {
		myPuts("No fue posible atender requerimientos de CPU");
		exit(1);
	}

	return 0;
}

void* connectionDAM(){
	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t servidor; // Descriptor de socket a la escucha
	int sock_Cliente;
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfig("IP_ESCUCHA","S-AFA.txt",0));
	PUERTO_ESCUCHA=(int) getConfig("DAM_PUERTO","S-AFA.txt",1);


	result = myEnlazarServidor((int*) &servidor, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos DAM");
		exit(1);
	}

	result = myAtenderCliente((int*)&servidor, "S-AFA", "DAM", &sock_Cliente);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de DAM");
		exit(1);
	}

	gestionarConexionDAM();

	return 0;
}

int main(void)
{
	char *linea;
	pthread_t hiloConnectionCPU; //Nombre de Hilo a crear
	pthread_t hiloConnectionDAM; //Nombre de Hilo a crear


	pthread_create(&hiloConnectionDAM,NULL,(void*) &connectionDAM,NULL);
	pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);


	while (1) {
		linea = readline(">");
		if (linea)
			add_history(linea);

		if(!strncmp(linea,"config",6))
		{
			mostrarConfig();
		}

		if(!strncmp(linea,"get",3))
		{
			char** split;
			char token[20];
			split = string_split(linea, " ");

			strcpy(token, split[1]);
			printf("%s",(char*)getConfig(token,"S-AFA.txt",0));

		}

		if(!strncmp(linea,"ejecutar ",9))
		{
			char** split;
			char path[256];
			split = string_split(linea, " ");

			strcpy(path, split[1]);
			printf("La ruta del Escriptorio a ejecutar es: %s\n",path);

		}

		if(!strncmp(linea,"status",6))
		{
			char** split;
			char idS[3]; //Asumiendo que como maximo es 999
			int id = -1;
			split = string_split(linea, " ");

			if(split[1] == NULL){
				printf("Se mostrara el estado de las colas y la informacion complementaria\n");
			} else {
				strcpy(idS, split[1]);
				id = atoi(idS);
				printf("Se mostrara todos los datos almacenados en el DTB con ID: %d\n",id);
			}
		}

		if(!strncmp(linea,"finalizar ",10))
		{
			char** split;
			char idS[3]; //Asumiendo que como maximo es 999
			int id = -1;;
			split = string_split(linea, " ");

			strcpy(idS,"0");

			strcpy(idS, split[1]);
			id = atoi(idS);
			printf("Se manda al DTB con ID %d a la cola de EXIT\n",id);

		}

		if(!strncmp(linea,"metricas",8))
		{
			char** split;
			char idS[3]; //Asumiendo que como maximo es 999
			int id = -1;
			split = string_split(linea, " ");

			if(split[1] == NULL){
				printf("Se mostraran las metricas\n");
			} else {
				strcpy(idS, split[1]);
				id = atoi(idS);
				printf("Se mostraran las metricas para el DTB con ID: %d\n",id);
			}
		}

		free(linea);
	}
	return EXIT_SUCCESS;
}
