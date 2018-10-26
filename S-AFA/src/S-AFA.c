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
#include <parser/parser.h>
#include <dtbSerializacion/dtbSerializacion.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>

t_list *listaSentencias;
int	nroSentenciaActual;

const int OPERACION_ABRIR = 1;
const int OPERACION_CONCENTRAR = 2;
const int OPERACION_ASIGNAR = 3;
const int OPERACION_WAIT = 4;
const int OPERACION_SIGNAL = 5;
const int OPERACION_FLUSH = 6;
const int OPERACION_CLOSE = 7;
const int OPERACION_CREAR = 8;
const int OPERACION_BORRAR = 9;

typedef struct sentencia {
	int operacion;
	char *param1;
	int param2;
	char *param3;
} sentencia;

#define PATHCONFIGSAFA "/home/utnso/tp-2018-2c-smlc/Config/S-AFA.txt"
//t_config *configSAFA;

int IDGlobal = 0;
int DTBenPCP = 0;
int estadoSistema = -1; // 0 = Estado Operativo

int GsocketCPU;
int GsocketDAM;

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

t_queue *colaNEW;
t_queue *colaREADY;
t_queue *colaEXEC;
t_queue *colaBLOCK;
t_queue *colaEXIT;

static sem_t semCPU;
static sem_t semDAM;
static bool conectionDAM;
static int conectionCPU=0;

DTB* buscarDTBPorID(t_queue *miCola,int idGDT){
	for (int indice = 0;indice < list_size(miCola->elements);indice++){
			DTB *miDTB= list_get(miCola->elements,indice);
			if(miDTB->ID_GDT==idGDT){
				return miDTB;
			}
	}
	return NULL;
}

int buscarIndicePorIdGDT(t_queue *miCola,int idGDT){
	for (int indice = 0;indice < list_size(miCola->elements);indice++){
		DTB *miDTB= list_get(miCola->elements,indice);
		if(miDTB->ID_GDT==idGDT){
			return indice;
		}
	}
	return -1;
}

	///PLANIFICACION A LARGO PLAZO///

DTB* crearDTB(char *rutaMiScript){
	DTB *miDTB; //Sin free por que sino cuando lo meto en la cola pierdo el elemento
	miDTB = malloc(sizeof(DTB));

	miDTB->ID_GDT = IDGlobal;
	strcpy(miDTB->Escriptorio,rutaMiScript);
	miDTB->PC = 0;
	miDTB->Flag_EstadoGDT = 1;
	miDTB->tablaArchivosAbiertos = list_create();

	IDGlobal++;

	return miDTB;
}

void planificarNewReady(){
	while((!queue_is_empty(colaNEW)) && (DTBenPCP < configModificable.gradoMultiprogramacion)){
		/*el PLP eligirá uno de los DTBs en “NEW”, según su algoritmo de planificación,
		  y se comunicará con el PCP para indicarle que “desbloquee” el	DTB_Dummy.*/

		if(strcmp(configModificable.algoPlani,"FIFO") == 0 ){
			DTB *auxDTB;
			//DTB *aux2DTB;

			auxDTB = queue_pop(colaNEW);

			PCP(auxDTB);
			DTBenPCP++;
		} else if(strcmp(configModificable.algoPlani,"RR") == 0 ){

		} else if(strcmp(configModificable.algoPlani,"VRR") == 0 ){

		} else if(strcmp(configModificable.algoPlani,"PROPIO") == 0 ){

		}
	}
}

void PLP(char *rutaScript){
	DTB *elDTB;

	elDTB = crearDTB(rutaScript);
	queue_push(colaNEW,elDTB);

	planificarNewReady();
}

	///PLANIFICACION A CORTO PLAZO///

void recibirBloqueoDeDTByBloquearlo(){
	char buffer[5];
	int bloquear = 1; // Si es 0 hay que bloquear, si es 1 no

	myRecibirDatosFijos(GsocketCPU,buffer, sizeof(buffer));

	//myPuts("Buffer %s\n",buffer);

	if(strcmp(buffer,"BLOCK") == 0){ //Quiere decir que la operacion se ejecuto bien en la instancia entonces afecto la variable global
		bloquear = 0;
	}

	DTB *miDTB;

		miDTB = recibirDTB(GsocketCPU);

		//myPuts("El DTB que se recibio es:\n");
		//imprimirDTB(miDTB);

		queue_push(colaBLOCK,miDTB);
}

void PCP(DTB *miDTB){
	/*Este desbloqueo se efectuará indicando al DTB_Dummy en su contenido un flag de inicialización en
		0, el ID del DTB a “pasar” a “READY” y el path del script Escriptorio a cargar a memoria.*/

	char* strDTB;

	miDTB->Flag_EstadoGDT = 0;

	//Mandar a CPU y ver si va a READY o EXEC
	strDTB = DTBStruct2String (miDTB);

	myEnviarDatosFijos(GsocketCPU, strDTB, strlen(strDTB));

	/*Esta operación dummy consta de solicitarle a El Diego que busque en el MDJ el Escriptorio indicado
	en el DTB. Una vez realizado esto, el CPU desaloja a dicho DTB Dummy, avisando a S-AFA que debe
	bloquearlo. Si S-AFA recibe el DTB con el flag de inicialización en 0, moverá el DTB asociado al
	programa G.DT que se había ejecutado en la cola de Bloqueados.
	Cuando El Diego finaliza su operatoria, le comunicará a S-AFA si pudo o no alojar el Escriptorio en
	FM9, para que el primero pueda pasarlo a la cola de Ready o Exit (según como corresponda).*/

	recibirBloqueoDeDTByBloquearlo();
	//queue_push(colaREADY,miDTB);
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

///PARSEAR SCRIPTS///

void imprimirSentencia(sentencia *miEntrada)
{
    if (miEntrada->operacion == OPERACION_ASIGNAR){
    	myPuts("Operacion: %d Param1: %s Param2: %d Param3: %s\n",miEntrada->operacion,miEntrada->param1, miEntrada->param2, miEntrada->param3);
    }
    else if (miEntrada->operacion == OPERACION_CREAR){
        	myPuts("Operacion: %d Param1: %s Param2: %d\n",miEntrada->operacion, miEntrada->param1, miEntrada->param2);
    }
    else if (miEntrada->operacion == OPERACION_CONCENTRAR){
        	myPuts("Operacion: %d\n",miEntrada->operacion);
    }
    else {
    	myPuts("Operacion: %d Param1: %s\n",miEntrada->operacion, miEntrada->param1);
    }
}

void parsear(char *nombreArchivo){
	FILE * fdScript;
	char * line = NULL;
	size_t len = 0;
	size_t read;
	sentencia *laSentencia;

	fdScript = fopen((char*)nombreArchivo, "a+");
	if (fdScript == NULL){
		perror("Error al abrir el archivo: ");
		exit(EXIT_FAILURE);
	}

	listaSentencias = list_create();
	nroSentenciaActual = -1;

	while ((read = getline(&line, &len, fdScript)) != -1) {
		t_parser_operacion parsed = parse(line);

		laSentencia = malloc(sizeof(sentencia));
		laSentencia->operacion = 0;
		laSentencia->param1 = NULL;
		laSentencia->param2 = -1;
		laSentencia->param3 = NULL;

		if(parsed.valido){
			switch(parsed.keyword){
				case ABRIR:
					laSentencia->operacion = OPERACION_ABRIR;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.ABRIR.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.ABRIR.param1);
					laSentencia->param1[strlen(parsed.argumentos.ABRIR.param1)] = '\0';
					break;
				case CONCENTRAR:
					laSentencia->operacion = OPERACION_CONCENTRAR;
					break;
				case ASIGNAR:
					laSentencia->operacion = OPERACION_ASIGNAR;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.ASIGNAR.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.ASIGNAR.param1);
					laSentencia->param1[strlen(parsed.argumentos.ASIGNAR.param1)] = '\0';
					laSentencia->param2 = parsed.argumentos.ASIGNAR.param2;
					laSentencia->param3 = malloc(strlen(parsed.argumentos.ASIGNAR.param3)+1);
					strcpy(laSentencia->param3,parsed.argumentos.ASIGNAR.param3);
					laSentencia->param3[strlen(parsed.argumentos.ASIGNAR.param3)] = '\0';
					break;
				case WAIT:
					laSentencia->operacion = OPERACION_WAIT;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.WAIT.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.WAIT.param1);
					laSentencia->param1[strlen(parsed.argumentos.WAIT.param1)] = '\0';
					break;
				case SIGNAL:
					laSentencia->operacion = OPERACION_SIGNAL;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.SIGNAL.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.SIGNAL.param1);
					laSentencia->param1[strlen(parsed.argumentos.SIGNAL.param1)] = '\0';
					break;
				case FLUSH:
					laSentencia->operacion = OPERACION_FLUSH;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.FLUSH.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.FLUSH.param1);
					laSentencia->param1[strlen(parsed.argumentos.FLUSH.param1)] = '\0';
					break;
				case CLOSE:
					laSentencia->operacion = OPERACION_CLOSE;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.CLOSE.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.CLOSE.param1);
					laSentencia->param1[strlen(parsed.argumentos.CLOSE.param1)] = '\0';
					break;
				case CREAR:
					laSentencia->operacion = OPERACION_CREAR;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.CREAR.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.CREAR.param1);
					laSentencia->param1[strlen(parsed.argumentos.CREAR.param1)] = '\0';
					laSentencia->param2 = parsed.argumentos.CREAR.param2;
					break;
				case BORRAR:
					laSentencia->operacion = OPERACION_BORRAR;
					laSentencia->param1 = malloc(strlen(parsed.argumentos.BORRAR.param1)+1);
					strcpy(laSentencia->param1,parsed.argumentos.BORRAR.param1);
					laSentencia->param1[strlen(parsed.argumentos.BORRAR.param1)] = '\0';
					break;
				default:
					fprintf(stderr, "No pude interpretar <%s>\n", line);
					exit(EXIT_FAILURE);
			}
		} else {
			fprintf(stderr, "La linea <%s> no es valida\n", line);
			exit(EXIT_FAILURE);
		}

		list_add(listaSentencias,laSentencia);
	}

	//fclose(fdScript);
	if (line)
		free(line);

	myPuts("El archivo parseado es el siguiente: \n");
	list_iterate(listaSentencias,imprimirSentencia);
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
	GsocketCPU = *(int*)sock;

	conectionCPU++;
	if(conectionDAM==true && conectionCPU>0)
		estadoSistema = 0;
		myPuts("El proceso S-AFA esta en un estado OPERATIVO\n");

	/*//A modo de prueba solo para probar el envio de mensajes entre procesos, no tiene ninguna utilidad
		char buffer[5];
		strcpy(buffer,"hola");
		buffer[4]='\0';
		myEnviarDatosFijos(socketCPU,buffer,5);*/
}

void gestionarConexionDAM(int *sock_cliente){
	GsocketDAM = *(int*)sock_cliente;

	conectionDAM=true;
	//Creo que el if no tendria mucho sentiido ya que se debe conectar primero el DAM y luego CPU ya que se conecta con el,
	//por lo tanto es el quien debe validar el estado operativo
	/*if(conectionDAM==true && conectionCPU>0)
		myPuts("El proceso S-AFA esta en un estado OPERATIVO\n");*/

	/*//A modo de prueba, apra que a futuro se realice lo del reenvio de mensajes
	char buffer[5];
	strcpy(buffer,"hola");
	buffer[4]='\0';
	myEnviarDatosFijos(socketDAM,buffer,5);*/
}

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

		    free(split[0]);
		    free(split[1]);
		    free(split);
		}

		if(!strncmp(linea,"ejecutar ",9))
		{
			if(estadoSistema == 0){
				//Esta en un estado operativo
				char** split;
				char path[256];
				split = string_split(linea, " ");

				strcpy(path, split[1]);
				printf("La ruta del Escriptorio a ejecutar es: %s\n",path);
				//PLP(path);
				PLP(PATHCONFIGSAFA);

				   free(split[0]);
				   free(split[1]);
				   free(split);
			} else {
				myPuts("El sistema no se encuentra en un estado Operativo, espere a que alcance dicho estado y vuelva a intentar la operacion.\n");
			}
		}

		if(!strncmp(linea,"status",6))
		{
			char** split;
			char idS[3]; //Asumiendo que como maximo es 999
			int id = -1;
			split = string_split(linea, " ");

			if(split[1] == NULL){
				printf("Se mostrara el estado de las colas y la informacion complementaria\n");

				if (queue_is_empty(colaNEW)){
					printf("Cola NEW:\n");
					printf("No contiene DTB asignados.\n");
				} else {
					printf("Cola NEW:\n");
					int indice = 0;
					void imprimirIdDTB(DTB *miDTB){
					    myPuts("Posicion: %d ID_GDT: %d\n",indice,miDTB->ID_GDT);
					    indice++;
					}
					list_iterate(colaNEW->elements,imprimirIdDTB);
				}

				if (queue_is_empty(colaREADY)){
					printf("Cola READY:\n");
					printf("No contiene DTB asignados.\n");
				} else {
					printf("Cola READY:\n");
					int indice = 0;
					void imprimirIdDTB(DTB *miDTB){
					    myPuts("Posicion: %d ID_GDT: %d\n",indice,miDTB->ID_GDT);
					    indice++;
					}
					list_iterate(colaREADY->elements,imprimirIdDTB);
				}

				if (queue_is_empty(colaBLOCK)){
					printf("Cola BLOCK:\n");
					printf("No contiene DTB asignados.\n");
				} else {
					printf("Cola BLOCK:\n");
					int indice = 0;
					void imprimirIdDTB(DTB *miDTB){
					    myPuts("Posicion: %d ID_GDT: %d\n",indice,miDTB->ID_GDT);
					    indice++;
					}
					list_iterate(colaBLOCK->elements,imprimirIdDTB);
				}

				if (queue_is_empty(colaEXEC)){
					printf("Cola EXEC:\n");
					printf("No contiene DTB asignados.\n");
				} else {
					printf("Cola EXEC:\n");
					int indice = 0;
					void imprimirIdDTB(DTB *miDTB){
					    myPuts("Posicion: %d ID_GDT: %d\n",indice,miDTB->ID_GDT);
					    indice++;
					}
					list_iterate(colaEXEC->elements,imprimirIdDTB);
				}

				if (queue_is_empty(colaEXIT)){
					printf("Cola EXIT:\n");
					printf("No contiene DTB asignados.\n");
				} else {
					printf("Cola EXIT:\n");
					int indice = 0;
					void imprimirIdDTB(DTB *miDTB){
					    myPuts("Posicion: %d ID_GDT: %d\n",indice,miDTB->ID_GDT);
					    indice++;
					}
					list_iterate(colaEXIT->elements,imprimirIdDTB);
				}

			} else {
				strcpy(idS, split[1]);
				id = atoi(idS);

				DTB *miDTB;
				printf("Se mostrara todos los datos almacenados en el DTB con ID: %d\n",id);

				miDTB = buscarDTBPorID(colaNEW, id);
				if (miDTB == NULL){
					miDTB = buscarDTBPorID(colaREADY, id);
					if (miDTB == NULL){
						miDTB = buscarDTBPorID(colaBLOCK, id);
						if (miDTB == NULL){
							miDTB = buscarDTBPorID(colaEXEC, id);
							if (miDTB == NULL){
								miDTB = buscarDTBPorID(colaEXIT, id);
								if (miDTB == NULL){
									printf("El id %d es un id erroneo, no se encuentra en ninguna cola\n");
								} else {
									printf("Cola EXIT:\n");
									imprimirDTB(miDTB);
								}
							} else {
								printf("Cola EXEC:\n");
								imprimirDTB(miDTB);
							}
						} else {
							printf("Cola BLOCK:\n");
							imprimirDTB(miDTB);
						}
					} else {
						printf("Cola READY:\n");
						imprimirDTB(miDTB);
					}
				} else {
					printf("Cola NEW:\n");
					imprimirDTB(miDTB);
				}
			}

			free(split[0]);
			free(split[1]);
			free(split);
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

			DTB *miDTB;

			miDTB = buscarDTBPorID(colaNEW, id);
			if (miDTB == NULL){
				miDTB = buscarDTBPorID(colaREADY, id);
				if (miDTB == NULL){
					miDTB = buscarDTBPorID(colaBLOCK, id);
					if (miDTB == NULL){
						miDTB = buscarDTBPorID(colaEXEC, id);
						if (miDTB == NULL){
							miDTB = buscarDTBPorID(colaEXIT, id);
							if (miDTB == NULL){
								printf("El id %d es un id erroneo, no se encuentra en ninguna cola\n");
							} else {
								printf("El DTB con id %d ya se encuentra en la cola EXIT.\n",id);
							}
						} else {
							//printf("Se mando el DTB con ID %d a la cola de EXIT\n",id);
							printf("Cola EXEC:\n");
							//TURBIO//
							/* el G.DT se encuentra en la cola EXEC se deberá esperar a terminar la
							operación actual, para luego moverlo a la cola EXIT. */
						}
					} else {
						printf("Se mando el DTB con ID %d a la cola de EXIT\n",id);
						int indice;
						indice = buscarIndicePorIdGDT(colaBLOCK,id);
						miDTB = list_remove(colaBLOCK->elements, indice);
						queue_push(colaEXIT,miDTB);
					}
				} else {
					printf("Se mando el DTB con ID %d a la cola de EXIT\n",id);
					int indice;
					indice = buscarIndicePorIdGDT(colaREADY,id);
					miDTB = list_remove(colaREADY->elements, indice);
					queue_push(colaEXIT,miDTB);
				}
			} else {
				printf("Se mando el DTB con ID %d a la cola de EXIT\n",id);
				int indice;
				indice = buscarIndicePorIdGDT(colaNEW,id);
				miDTB = list_remove(colaNEW->elements, indice);
				queue_push(colaEXIT,miDTB);
			}


			free(split[0]);
			free(split[1]);
			free(split);

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

			free(split[0]);
			free(split[1]);
		}

		if(!strncmp(linea,"estado",6))
		{
			if(conectionDAM==true && conectionCPU>0){
				myPuts("El proceso S-AFA esta en un estado OPERATIVO\n");
			}else{
				myPuts("El proceso S-AFA esta en un estado CORRUPTO\n");
			}
		}
		if(!strncmp(linea,"pruebaParser",12))
		{
			char *miPath = "/home/utnso/tp-2018-2c-smlc/S-AFA/Debug/miScript.txt";
			parsear((char *)miPath);
		}
		if(!strncmp(linea,"exit",4))
		{
			exit(1);
		}
		free(linea);
	}
	return EXIT_SUCCESS;
}
