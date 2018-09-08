#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <conexiones/mySockets.h>
#include <console/myConsole.h>

#define PATHCONFIGSAFA "/home/utnso/tp-2018-2c-smlc/Config/S-AFA.txt"
t_config *configSAFA;

int IDGlobal = -1;
int DTBenPCP = 0;

char algoPlani[6];
int quantum;
int gradoMultiprogramacion;
int retardoPlani;

typedef struct DT_Block {
	int ID_GDT;
	char Escriptorio[256]; //El valor despues hay que velo bien cuando nos den un ejemplo de Sript
	int PC;
	int Flag_EstadoGDT;
	t_list *tablaArchivosAbiertos;
} DTB;

t_queue *colaNEW;
t_queue *colaREADY;
t_queue *colaEXEC;
t_queue *colaBLOCK;
t_queue *colaEXIT;

static sem_t semCPU;
static sem_t semDAM;
static bool conectionDAM;
static int conectionCPU=0;

	///PLANIFICACION A LARGO PLAZO///

DTB* crearDTB(char *rutaMiScript){
	DTB *miDTB;
	miDTB = malloc(sizeof(DTB));

	miDTB->ID_GDT = IDGlobal + 1;
	strcpy(miDTB->Escriptorio,rutaMiScript);
	miDTB->PC = 0;
	miDTB->Flag_EstadoGDT = 1;
	miDTB->tablaArchivosAbiertos = list_create();

	IDGlobal++;

	return miDTB;
}

void planificarNewReady(){
	while((!queue_is_empty(colaNEW)) && (DTBenPCP < gradoMultiprogramacion)){
		DTB *auxDTB;

		auxDTB = queue_pop(colaNEW);
		//Mandar a CPU y ver si va a READY o EXEC
		queue_push(colaREADY,auxDTB);

		DTBenPCP++;
	}
}

void PLP(char *rutaScript){
	DTB *elDTB;

	elDTB = crearDTB(rutaScript);
	queue_push(colaNEW,elDTB);
	free(elDTB);

	planificarNewReady();


}

	///PLANIFICACION A CORTO PLAZO///

void PCP(){

}

	///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("DAM -> IP: %s - Puerto: %s \0", (char*)getConfigR("IP_ESCUCHA",0,configSAFA),(char*)getConfigR("DAM_PUERTO",0,configSAFA) );
	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("CPU -> IP: %s - Puerto: %s \0", (char*)getConfigR("IP_ESCUCHA",0,configSAFA),(char*)getConfigR("CPU_PUERTO",0,configSAFA) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Algoritmo: %s \0", (char*)getConfigR("ALGO_PLANI",0,configSAFA) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Grado de multiprogramacion: %s \0", (char*)getConfigR("GMP",0,configSAFA) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Quantum: %s \0", (char*)getConfigR("Q",0,configSAFA) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Retardo: %s \0" COLOR_RESET , (char*)getConfigR("RETARDO",0,configSAFA) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

void cargarValoresConfig(char* algo,int *q,int *gMp,int *retardo){
	strcpy(algo,(char*)getConfigR("ALGO_PLANI",0,configSAFA));
	*q=(int*)getConfigR("Q",0,configSAFA);
	*gMp=(int*)getConfigR("GMP",0,configSAFA);
	*retardo=(int*)getConfigR("RETARDO",0,configSAFA);
}

	///FUNCIONES DE INICIALIZACION///

void inicializarSemaforos(){
	sem_init(&semCPU,0,0);
	sem_init(&semDAM,0,0);
}

void creacionDeColas(){
	colaNEW = queue_create();
	colaREADY = queue_create();
	colaEXEC = queue_create();
	colaBLOCK = queue_create();
	colaEXIT = queue_create();
}

	///GESTION DE CONEXIONES///

void gestionarConexionCPU(int* sock){
	int socketCPU = *(int*)sock;
	conectionCPU++;
	if(conectionDAM==true && conectionCPU>0)
		myPuts("El proceso S-AFA esta en un estado OPERATIVO\n");

	//A modo de prueba solo para probar el envio de mensajes entre procesos, no tiene ninguna utilidad
		char buffer[5];
		strcpy(buffer,"hola");
		buffer[4]='\0';
		myEnviarDatosFijos(socketCPU,buffer,5);
}

void gestionarConexionDAM(int *sock_cliente){
	int socketDAM = *(int*)sock_cliente;

	conectionDAM=true;
	if(conectionDAM==true && conectionCPU>0)
		myPuts("El proceso S-AFA esta en un estado OPERATIVO\n");

	//A modo de prueba, apra que a futuro se realice lo del reenvio de mensajes
	char buffer[5];
	strcpy(buffer,"hola");
	buffer[4]='\0';
	myEnviarDatosFijos(socketDAM,buffer,5);
}

	///FUNCIONES DE CONEXION///

	///FUNCIONES DE CONEXION///
void* connectionCPU() {

	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t servidor; // Descriptor de socket a la escucha
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfigR("IP_ESCUCHA",0,configSAFA));
	PUERTO_ESCUCHA=(int) getConfigR("CPU_PUERTO",1,configSAFA);

	result = myEnlazarServidor((int*) &servidor, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos CPU");
		exit(1);
	}

	result = myAtenderClientesEnHilos((int*) &servidor, "S-AFA", "CPU", gestionarConexionCPU);

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
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfigR("IP_ESCUCHA",0,configSAFA));
	PUERTO_ESCUCHA=(int) getConfigR("DAM_PUERTO",1,configSAFA);


	result = myEnlazarServidor((int*) &servidor, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos DAM");
		exit(1);
	}

	result = myAtenderClientesEnHilos((int*) &servidor, "S-AFA", "DAM", gestionarConexionDAM);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de DAM");
		exit(1);
	}

	return 0;
}

	///MAIN///

int main(void)
{
	system("clear");
	char *linea;
	pthread_t hiloConnectionCPU; //Nombre de Hilo a crear
	pthread_t hiloConnectionDAM; //Nombre de Hilo a crear

	configSAFA=config_create(PATHCONFIGSAFA);

	creacionDeColas();
	//cargarValoresConfig(algoPlani, quantum, gradoMultiprogramacion, retardoPlani);


	pthread_create(&hiloConnectionDAM,NULL,(void*) &connectionDAM,NULL);
	pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);

//TODO Free de split y thread notify
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
			PLP(path);

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

		if(!strncmp(linea,"estado",6))
		{
			if(conectionDAM==true && conectionCPU>0){
				myPuts("El proceso S-AFA esta en un estado OPERATIVO\n");
			}else{
				myPuts("El proceso S-AFA esta en un estado CORRUPTO\n");
			}
		}
		if(!strncmp(linea,"exit",4))
		{
			exit(1);
		}
		free(linea);
	}
	return EXIT_SUCCESS;
}
