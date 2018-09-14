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
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>

#define PATHCONFIGSAFA "/home/utnso/tp-2018-2c-smlc/Config/S-AFA.txt"
//t_config *configSAFA;

int IDGlobal = -1;
int DTBenPCP = 0;

typedef struct ModiConfig {
	char IP_ESCUCHA[15];
	int PUERTO_DAM;
	int PUERTO_CPU;
	char algoPlani[6];
	int quantum;
	int gradoMultiprogramacion;
	int retardoPlani;
} ModiConfig;

ModiConfig configModificable;

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
	while((!queue_is_empty(colaNEW)) && (DTBenPCP < configModificable.gradoMultiprogramacion)){
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

	char* myText = string_from_format("DAM -> IP: %s - Puerto: %d \0", configModificable.IP_ESCUCHA,configModificable.PUERTO_DAM);
		displayBoxTitle(50,"CONFIGURACION");
		displayBoxBody(50,myText);
		displayBoxClose(50);
		myText = string_from_format("CPU -> IP: %s - Puerto: %d \0", configModificable.IP_ESCUCHA,configModificable.PUERTO_CPU);
		displayBoxBody(50,myText);
		displayBoxClose(50);
		myText = string_from_format("Algoritmo: %s \0", configModificable.algoPlani);
		displayBoxBody(50,myText);
		displayBoxClose(50);
		myText = string_from_format("Grado de multiprogramacion: %d \0", configModificable.gradoMultiprogramacion);
		displayBoxBody(50,myText);
		displayBoxClose(50);
		myText = string_from_format("Quantum: %d \0", configModificable.quantum);
		displayBoxBody(50,myText);
		displayBoxClose(50);
		myText = string_from_format("Retardo: %d \0" COLOR_RESET , configModificable.retardoPlani);
		displayBoxBody(50,myText);
		displayBoxClose(50);
	    free(myText);
}

void actualizarConfig(){
	t_config *configAux= config_create(PATHCONFIGSAFA);

	strcpy(configModificable.algoPlani,config_get_string_value(configAux,"ALGO_PLANI"));
	configModificable.quantum=config_get_int_value(configAux,"Q");
	configModificable.gradoMultiprogramacion=config_get_int_value(configAux,"GMP");
	configModificable.retardoPlani=config_get_int_value(configAux,"RETARDO");

	config_destroy(configAux);
}

void cargarConfig(){
	t_config *configSAFA= config_create(PATHCONFIGSAFA);

	strcpy(configModificable.IP_ESCUCHA,config_get_string_value(configSAFA,"IP_ESCUCHA"));
	configModificable.PUERTO_DAM=config_get_int_value(configSAFA,"DAM_PUERTO");
	configModificable.PUERTO_CPU=config_get_int_value(configSAFA,"CPU_PUERTO");

	config_destroy(configSAFA);

	actualizarConfig();

}

/*MONITOR DE MODIFICACION DE CONFIG*/
void notifyConfig(){

#define EVENT_SIZE  ( sizeof (struct inotify_event) + 16 )
#define BUF_LEN     ( 1024 * EVENT_SIZE )

	char buffer[BUF_LEN];
	int file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		perror("inotify_init");
	}

	int watch_descriptor = inotify_add_watch(file_descriptor, "../../Config", IN_MODIFY | IN_CREATE | IN_DELETE);

	while (1){
			int length = read(file_descriptor, buffer, BUF_LEN);
			if (length < 0) {
				perror("read");
			}

			int offset = 0;

			// Luego del read buffer es un array de n posiciones donde cada posición contiene
			// un eventos ( inotify_event ) junto con el nombre de este.
			while (offset < length) {

				// El buffer es de tipo array de char, o array de bytes. Esto es porque como los
				// nombres pueden tener nombres mas cortos que 24 caracteres el tamaño va a ser menor
				// a sizeof( struct inotify_event ) + 24.
				struct inotify_event *event = (struct inotify_event *) &buffer[offset];

				// El campo "len" nos indica la longitud del tamaño del nombre
				if (event->len) {
					// Dentro de "mask" tenemos el evento que ocurrio y sobre donde ocurrio
					// sea un archivo o un directorio
					if (event->mask & IN_MODIFY) {
						if (!(event->mask & IN_ISDIR)) {
							if(strcmp(event->name,"S-AFA.txt") == 0){
								printf("Se modifico el archivo %s.\n", event->name);
								actualizarConfig();
							}
						}
					}
				}
				offset += sizeof (struct inotify_event) + event->len;
			}
		}
		inotify_rm_watch(file_descriptor, watch_descriptor);
		close(file_descriptor);
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

	result = myEnlazarServidor((int*) &servidor, &direccionServidor,configModificable.IP_ESCUCHA,configModificable.PUERTO_CPU); // Obtener socket a la escucha
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

	result = myEnlazarServidor((int*) &servidor, &direccionServidor,configModificable.IP_ESCUCHA,configModificable.PUERTO_DAM); // Obtener socket a la escucha
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
	//pthread_t hiloNotifyConfig; //Nombre de Hilo a crear

	//configSAFA=config_create(PATHCONFIGSAFA);

	creacionDeColas();
	cargarConfig();


	pthread_create(&hiloConnectionDAM,NULL,(void*) &connectionDAM,NULL);
	pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);
	//pthread_create(&hiloNotifyConfig,NULL,(void*)&notifyConfig,NULL);

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
