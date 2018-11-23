#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/string.h>
#include <conexiones/mySockets.h>
#include <console/myConsole.h>
#include <dtbSerializacion/dtbSerializacion.h>
#include <netinet/in.h>
#include <parser/parser.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <semaphore.h>
#include <sentencias/sentencias.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

#define PATHCONFIGSAFA "/home/utnso/tp-2018-2c-smlc/Config/S-AFA.txt"
//t_config *configSAFA;


int IDGlobal = 0;
int DTBenPCP = 0;
int estadoSistema = -1; // 0 = Estado Operativo
int cantHistoricaDeSentencias = 0;
int cantDeSentenciasQueUsaronADiego = 0;
int GsocketDAM;
int finalizarPorConsola = -1;

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

typedef struct metrica {
	int ID_DTB;
	int sentenciasEjecutadasEnNEW;
	int sentenciasEjecutadasHastaEXIT;
} metrica;

typedef struct DTBPrioridadVRR{
	DTB* unDTB;
	int remanente;
} DTBPrioridadVRR;

typedef struct clienteCPU {
	int socketCPU;
	int libre; // 1 si 0 no
	int idDTB;
} clienteCPU;

typedef struct recurso {
	char* recurso;
	int semaforo;
	t_list *DTBWait;
	t_list *DTBBloqueados;
} recurso;

t_queue *colaNEW;
t_queue *colaREADY;
t_queue *colaEXEC;
t_queue *colaBLOCK;
t_queue *colaEXIT;
t_queue *colaVRR;

t_list *listaSentencias;
t_list *listaMetricas;
t_list *listaCPU;
t_list *listaProcesosAFinalizar;
t_list *listaRecursos;

static sem_t semCPU;
static sem_t semDAM;
static bool conectionDAM = false;

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


//FUNCIONES PARA LA LISTAS/COLAS
static void dtb_destroy(DTB* self){
	printf("Me libere\n");
	list_destroy(self->tablaArchivosAbiertos);
	free(self);
}

DTB* crearDTB(char *rutaMiScript){
	DTB *miDTB; //Sin free por que sino cuando lo meto en la cola pierdo el elemento
	miDTB = malloc(sizeof(DTB));

	miDTB->ID_GDT = IDGlobal;
	strcpy(miDTB->Escriptorio,rutaMiScript);
	miDTB->PC = 0;
	miDTB->Flag_GDTInicializado = 1;
	miDTB->tablaArchivosAbiertos = list_create();

	IDGlobal++;

	return miDTB;
}

metrica* crearMetricasParaDTB(int IDDTB){
	metrica *miMetrica;
	miMetrica = malloc(sizeof(metrica));

	miMetrica->ID_DTB = IDDTB;
	miMetrica->sentenciasEjecutadasEnNEW = cantHistoricaDeSentencias;
	miMetrica->sentenciasEjecutadasHastaEXIT = cantHistoricaDeSentencias;

	return miMetrica;
}

DTBPrioridadVRR *crearDTBVRR(DTB* miDTB, int sobra){
	DTBPrioridadVRR *miDTBVRR;
	miDTBVRR = malloc(sizeof(DTBPrioridadVRR));

	miDTBVRR->unDTB = miDTB;
	miDTBVRR->remanente = sobra;

	return miDTBVRR;
}

clienteCPU* crearClienteCPU(int socket){
	clienteCPU *nuevoCPU;
	nuevoCPU = malloc(sizeof(clienteCPU));

	nuevoCPU->socketCPU = socket;
	nuevoCPU->libre =1;

	return nuevoCPU;

}

recurso* crearRecurso(char* strRecurso){
	recurso *miRecurso;
	miRecurso = malloc(sizeof(recurso));

	strcpy(miRecurso->recurso,strRecurso);

	miRecurso->semaforo = 1;

	miRecurso->DTBWait = list_create();
	miRecurso->DTBBloqueados = list_create();

	return miRecurso;
}

bool hayCPUDisponible(){
	bool estaLibre(clienteCPU *unaCPU) {
		return unaCPU->libre == 1;
	}
	return list_any_satisfy(listaCPU, (void*) estaLibre);

}

clienteCPU* buscarCPUporSock(int sockCPU){
	for(int i = 0 ; i < list_size(listaCPU); i++){

		clienteCPU * elemento;

		elemento = list_get(listaCPU,i);

		if(elemento->socketCPU == sockCPU)
			return elemento;
	}

	return NULL;
}

clienteCPU* buscarCPUporIdDTB(int miID){
	for(int i = 0 ; i < list_size(listaCPU); i++){

		clienteCPU * elemento;

		elemento = list_get(listaCPU,i);

		if(elemento->idDTB == miID)
			return elemento;
	}

	return NULL;
}

clienteCPU* buscarCPUDisponible(){

	for(int i = 0 ; i < list_size(listaCPU); i++){

		clienteCPU * elemento;

		elemento = list_get(listaCPU,i);

		if(elemento->libre == 1)
			return elemento;
	}

	return NULL;

}

DTB* buscarDTBPorID(t_queue *miCola,int idGDT){
	for (int indice = 0;indice < list_size(miCola->elements);indice++){
			DTB *miDTB= list_get(miCola->elements,indice);
			if(miDTB->ID_GDT==idGDT){
				return miDTB;
			}
	}
	return NULL;
}

int buscarIndicePorIdGDT(t_list *miLista,int idGDT){
	for (int indice = 0;indice < list_size(miLista);indice++){
		DTB *miDTB= list_get(miLista,indice);
		if(miDTB->ID_GDT==idGDT){
			return indice;
		}
	}
	return -1;
}

bool debeFinalizarse(int idDTB){
	bool esElDTB(int unID) {
		return unID == idDTB;
	}

	return list_any_satisfy(listaProcesosAFinalizar, (void*) esElDTB);
}

int buscarRecurso(char* recursoABuscar){
	for (int indice = 0;indice < list_size(listaRecursos);indice++){
		recurso *miRecurso= list_get(listaRecursos,indice);
		if(strcmp(miRecurso->recurso,recursoABuscar)==0){
			return indice;
		}
	}
	return -1;
}

void mostrarCPUs(){

	for(int i = 0; i < list_size(listaCPU); i++){
		clienteCPU * cpu = list_get(listaCPU,i);

		myPuts("Socket CPU: %d \n",cpu->socketCPU);
	}
}

	///FUNCIONES METRICAS///

void actualizarMetricaEXIT(int idDTB){

	bool esMetricaDelDTB(metrica *metricaAux){
		return metricaAux->ID_DTB == idDTB;
	}

	metrica *laMetrica;
	laMetrica = list_find(listaMetricas, (void*) esMetricaDelDTB);

	laMetrica->sentenciasEjecutadasHastaEXIT = cantHistoricaDeSentencias - laMetrica->sentenciasEjecutadasHastaEXIT;
}

void actualizarMetricaNEW(int idDTB){

	bool esMetricaDelDTB(metrica *metricaAux){
		return metricaAux->ID_DTB == idDTB;
	}

	metrica *laMetrica;
	laMetrica = list_find(listaMetricas, (void*) esMetricaDelDTB);

	laMetrica->sentenciasEjecutadasEnNEW = cantHistoricaDeSentencias - 	laMetrica->sentenciasEjecutadasEnNEW;
}

	///ENVIAR Y RECIBIR MENSAJES PARA LA PLANIFICACION///

void enviarQyPlanificacionACPU(int CPU){
	char planificacion[5];
	int quantum = configModificable.quantum;

	if(strcmp(configModificable.algoPlani,"FIFO")==0){
		quantum = -1;
	}

	strcpy(planificacion,configModificable.algoPlani);
	planificacion[4]='\0';
	myEnviarDatosFijos(CPU,planificacion,5);

	myEnviarDatosFijos(CPU,&quantum,sizeof(int));

}

void recibirDesbloqueoDAM() {
	int aperturaEscriptorio; // 1 abierto 0 error de apertura
	DTB * miDTB;

	myRecibirDatosFijos(GsocketDAM,&aperturaEscriptorio,sizeof(int));

	miDTB = recibirDTB(GsocketDAM);

	if (aperturaEscriptorio == 1){
		int indice = buscarIndicePorIdGDT(colaBLOCK->elements,miDTB->ID_GDT);
		list_remove(colaBLOCK->elements, indice);

		miDTB->Flag_GDTInicializado = 1;
		DTBenPCP++;

		queue_push(colaREADY,miDTB);

		actualizarMetricaNEW(miDTB->ID_GDT);
	} else if (aperturaEscriptorio == 0){
		int indice=buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
		list_remove(colaEXEC->elements, indice);

		actualizarMetricaEXIT(miDTB->ID_GDT);
		PLP();
	}
}

DTB* recibirDTBeInstrucciones(int socketCPU,int motivoLiberacionCPU){
	DTB *miDTB;
	clienteCPU *miCPU;

	miDTB = recibirDTB(socketCPU);

	int instruccionesRealizadas;
	myRecibirDatosFijos(socketCPU,&instruccionesRealizadas,sizeof(int));
	cantHistoricaDeSentencias += instruccionesRealizadas;

	miCPU = buscarCPUporSock(socketCPU);
	miCPU->libre = 1;

	if(!list_is_empty(listaProcesosAFinalizar)){
		if(debeFinalizarse(miDTB->ID_GDT)){
			int indice;
			indice= buscarIndicePorIdGDT(listaProcesosAFinalizar,miDTB->ID_GDT);
			list_remove(colaBLOCK->elements, indice);

			indice=buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
			list_remove(colaEXEC->elements, indice);

			printf("Se mando el DTB con ID %d a la cola de EXIT por el comando finalizar \n",finalizarPorConsola);

			finalizarDTB(miDTB);
			finalizarPorConsola=-1;
		}
	}else{
		accionSegunPlanificacion(miDTB,motivoLiberacionCPU,instruccionesRealizadas);
	}

	return miDTB;

}

	///PLANIFICACION A LARGO PLAZO///

void operacionDummy(DTB *miDTB){
	if(hayCPUDisponible() ) {
		char* strDTB;
		clienteCPU*CPULibre;

		miDTB->Flag_GDTInicializado = 0; //"Lo transforma en dummy"

		CPULibre = buscarCPUDisponible();
		int socketCPU = CPULibre->socketCPU;
		CPULibre->libre = 0; // Cpu ocupada
		CPULibre->idDTB = miDTB->ID_GDT;

		actualizarConfig();
		enviarQyPlanificacionACPU(socketCPU);

		queue_push(colaEXEC,miDTB);

		strDTB = DTBStruct2String (miDTB);
		myEnviarDatosFijos(CPULibre->socketCPU, strDTB, strlen(strDTB));

		int motivo;
		myRecibirDatosFijos(socketCPU,&motivo,sizeof(int));

		recibirDTBeInstrucciones(CPULibre->socketCPU,motivo);
		free(strDTB);
		PCP();
	}
}

void PLP(){

	while(!queue_is_empty(colaNEW)){

		actualizarConfig();

		if (DTBenPCP < configModificable.gradoMultiprogramacion){
			DTB *auxDTB;

			auxDTB = queue_pop(colaNEW);

			operacionDummy(auxDTB);
		}
	}
}

void NuevoDTByPlanificacion(char *rutaScript){
	DTB *elDTB;
	metrica *laMetrica;

	elDTB = crearDTB(rutaScript);
	queue_push(colaNEW,elDTB);

	laMetrica = crearMetricasParaDTB(elDTB->ID_GDT);
	list_add(listaMetricas,laMetrica);

	PLP();
}

///FINALIZAR DTB///

void verificarSiSePuedeLiberarUnRecurso(recurso *miRecurso){
	if(miRecurso->semaforo >= 0){
		if(!list_is_empty(miRecurso->DTBBloqueados)){
			DTB * DTBaDesbloquear = list_get(miRecurso->DTBBloqueados,0); //Lo saca de la cola de Bloqueados y lo pone en la de Wait/Asignado
			list_add(miRecurso->DTBWait, DTBaDesbloquear);
			miRecurso->semaforo -= 1;

			int indiceEXEC = buscarIndicePorIdGDT(colaBLOCK->elements,DTBaDesbloquear->ID_GDT);
			list_remove(colaBLOCK->elements,indiceEXEC);

			queue_push(colaREADY,DTBaDesbloquear);

		}
	}
}

void liberarRecursosAsignados(DTB *miDTB){
	if(!list_is_empty(listaRecursos)){

		for(int i; i < list_size(listaRecursos); i++){
			recurso * miRecurso = list_get(listaRecursos,i);

			int indice = buscarIndicePorIdGDT(miRecurso->DTBWait, miDTB->ID_GDT);
			if(indice != -1){
				miRecurso->semaforo += 1;
				list_remove(miRecurso->DTBWait,indice);
			}

			verificarSiSePuedeLiberarUnRecurso(miRecurso);
		}

	}
}

void finalizarDTB(DTB* miDTB){
	queue_push(colaEXIT,miDTB);

	actualizarMetricaEXIT(miDTB->ID_GDT);

	liberarRecursosAsignados(miDTB);

	DTBenPCP--;

	PLP();
}

	///PLANIFICACION CORTO PLAZO///

void accionSegunPlanificacion(DTB* miDTB, int motivoLiberacionCPU, int instruccionesRealizadas){
	int indice;
	int sobra;

	switch(motivoLiberacionCPU){

	case MOT_QUANTUM:
		indice=buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
		list_remove(colaEXEC->elements, indice);
		queue_push(colaREADY,miDTB);
	break;

	case MOT_BLOQUEO:
		cantDeSentenciasQueUsaronADiego += 1; // Todas las sentencias que se bloquearon usaron al DAM

		indice=buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
		list_remove(colaEXEC->elements, indice);

		if(strcmp(configModificable.algoPlani,"FIFO")==0 || strcmp(configModificable.algoPlani,"RR")==0){
			queue_push(colaBLOCK,miDTB);
		}

		sobra = configModificable.quantum - instruccionesRealizadas;

		if(strcmp(configModificable.algoPlani,"VRR") == 0 && sobra > 0){
			DTBPrioridadVRR * miDTBVRR;
			miDTBVRR = crearDTBVRR(miDTB,sobra);
			queue_push(colaVRR,miDTBVRR);
		}

	break;

	case MOT_FINALIZO:
		indice=buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
		list_remove(colaEXEC->elements, indice);

		finalizarDTB(miDTB);
	break;

	case MOT_ERROR:
		indice=buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
		list_remove(colaEXEC->elements, indice);

		finalizarDTB(miDTB);
	break;

	}

}

void asignarRecurso(int socketCPU, recurso *miRecurso, DTB *miDTB){
	int continuarEjecucion;

	if(miRecurso->semaforo >= 0 ){
		list_add(miRecurso->DTBWait,miDTB);
		continuarEjecucion = 1;
		myEnviarDatosFijos(socketCPU,&continuarEjecucion,sizeof(int)); // OK
	}else{
		list_add(miRecurso->DTBBloqueados,miDTB);
		myEnviarDatosFijos(socketCPU,0,sizeof(int)); // BloquearDTB
	}

}

void ejecutarAccionWaitSignal(int accion, int socketCPU,DTB *miDTB){
	recurso *miRecurso;
	int tamanio;
	int indiceRecurso;
	char * recurso = NULL;
	int continuarEjecucion;

	switch(accion){
	case ACC_WAIT:
		myRecibirDatosFijos(socketCPU,&tamanio,sizeof(int));
		myRecibirDatosFijos(socketCPU,recurso,tamanio);

		indiceRecurso = buscarRecurso(recurso);

		if(indiceRecurso == -1){
			miRecurso = crearRecurso(recurso);
			list_add(miRecurso->DTBWait,miDTB);

			continuarEjecucion = 1;
			myEnviarDatosFijos(socketCPU,&continuarEjecucion,sizeof(int)); // OK
		}else{
			miRecurso = list_get(listaRecursos,indiceRecurso);
			miRecurso->semaforo -= 1;
			asignarRecurso(socketCPU,miRecurso,miDTB);
		}

	break;

	case ACC_SIGNAL:
		myRecibirDatosFijos(socketCPU,&tamanio,sizeof(int));
		myRecibirDatosFijos(socketCPU,recurso,tamanio);

		continuarEjecucion = 1;
		myEnviarDatosFijos(socketCPU,&continuarEjecucion,sizeof(int)); // OK

		indiceRecurso = buscarRecurso(recurso);
		if(indiceRecurso == -1){
			miRecurso = crearRecurso(recurso);
		}else{
			miRecurso = list_get(listaRecursos,indiceRecurso);
			miRecurso->semaforo += 1;
			verificarSiSePuedeLiberarUnRecurso(miRecurso);
		}

	break;

	}

}

DTB * elegirProximoAEjecutarSegunPlanificacion(int socketCPU){
	DTB* miDTB;
	if(strcmp(configModificable.algoPlani,"FIFO")==0|| strcmp(configModificable.algoPlani,"RR")==0){

		myEnviarDatosFijos(socketCPU,0, sizeof(int)); //Remanente
		miDTB = queue_pop(colaREADY);

	}else if(strcmp(configModificable.algoPlani,"VRR")==0){
		if(!queue_is_empty(colaVRR)){
			DTBPrioridadVRR *DTBVRR;
			DTBVRR = queue_pop(colaVRR);

			int remanente= DTBVRR->remanente;
			miDTB = DTBVRR->unDTB;

			myEnviarDatosFijos(socketCPU, &remanente, sizeof(int));
		}else{
			myEnviarDatosFijos(socketCPU,0, sizeof(int)); //Remanente
			miDTB = queue_pop(colaREADY);

		}
	}
	return miDTB;
}

void PCP(){
	/*Este desbloqueo se efectuará indicando al DTB_Dummy en su contenido un flag de inicialización en
		0, el ID del DTB a “pasar” a “READY” y el path del script Escriptorio a cargar a memoria.*/
	while(hayCPUDisponible() && !queue_is_empty(colaREADY) &&! queue_is_empty(colaVRR)  ) {
		char* strDTB;

		clienteCPU*CPULibre;

		CPULibre = buscarCPUDisponible();
		int socketCPU = CPULibre->socketCPU;
		CPULibre->libre = 0;                // Cpu ocupada

		actualizarConfig();
		enviarQyPlanificacionACPU(socketCPU);

		DTB * miDTB;
		miDTB = elegirProximoAEjecutarSegunPlanificacion(socketCPU);

		CPULibre->idDTB = miDTB->ID_GDT;

		queue_push(colaEXEC, miDTB);

		strDTB = DTBStruct2String (miDTB);
		myEnviarDatosFijos(socketCPU, strDTB, strlen(strDTB));

		int motivo;
		myRecibirDatosFijos(socketCPU,&motivo,sizeof(int));

		while(motivo == ACC_SIGNAL || motivo == ACC_WAIT){
			ejecutarAccionWaitSignal(motivo,socketCPU,miDTB);

			myRecibirDatosFijos(socketCPU,&motivo,sizeof(int));
		}

		recibirDTBeInstrucciones(socketCPU,motivo);
	}
}

/*MONITOR DE MODIFICACION DE CONFIG*/
/*void notifyConfig(){

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
								for(int i = 0 ; i < list_size(listaCPU); i++){
									clienteCPU *miCPU;

									miCPU = list_get(listaCPU, i);

									int modo = MODO_QyP;
									myEnviarDatosFijos(miCPU->socketCPU,&modo, sizeof(int));
								}
							}
						}
					}
				}
				offset += sizeof (struct inotify_event) + event->len;
			}
		}
		inotify_rm_watch(file_descriptor, watch_descriptor);
		close(file_descriptor);
}*/

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
	colaVRR = queue_create();
}

void creacionDeListas(){
	listaMetricas =list_create();
	listaCPU = list_create();
	listaProcesosAFinalizar = list_create();
	listaRecursos = list_create();
}

	///GESTION DE CONEXIONES///

void desconectarCPU(int socketCPU){
	int indice;
	clienteCPU * clienteCPU;

	indice = buscarCPUporSock(socketCPU);
	clienteCPU = list_remove(listaCPU,indice);

	if(clienteCPU->libre == 0){
		int idDTB = clienteCPU->idDTB;

		printf("Se finalizo el DTB %d porque no se termino de forma correcta la ejecucion ", idDTB);

		DTB* miDTB = buscarDTBPorID(colaEXEC->elements,idDTB);

		finalizarDTB(miDTB);
	}

	free(clienteCPU);
}

void gestionarConexionCPU(int* sock){
	int socketCPU = *(int*)sock;

	clienteCPU *nuevoClienteCPU;

	nuevoClienteCPU =crearClienteCPU(socketCPU);

	list_add(listaCPU,nuevoClienteCPU);

	if(conectionDAM==true && !(list_is_empty(listaCPU))){
		estadoSistema = 0;

		if(list_size(listaCPU) == 1){

			myPuts("El proceso S-AFA esta en un estado OPERATIVO \n");
		}

	}

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

	int result;
	int accion;
	int socketCPUDesconectada;

	while(1){

		result = myRecibirDatosFijos(GsocketDAM,&accion,sizeof(int));

		if(result != 1){
			switch(accion){
				case DESCONEXION_CPU:
					myRecibirDatosFijos(GsocketDAM,&socketCPUDesconectada,sizeof(int));

					printf("Se desconecto la CPU NRO %d \n", socketCPUDesconectada);

					desconectarCPU(socketCPUDesconectada);

				break;

			}
		}else{
			conectionDAM = false;

			printf("Se desconecto el DAM, No estoy en estado operativo, Chau!!\n");

			exit(1);
		}
	}

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
	creacionDeListas();
	cargarConfig();

	pthread_create(&hiloConnectionDAM,NULL,(void*) &connectionDAM,NULL);
	pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);
	//pthread_create(&hiloNotifyConfig,NULL,(void*)&notifyConfig,NULL);

	while (1) {
		linea = readline(">");
		if (linea)
			add_history(linea);

		add_history("ejecutar Archivos/scripts/checkpoint.escriptorio");

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
				// NuevoDTByPlanificacion(path);
				 NuevoDTByPlanificacion(path);

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
					list_iterate(colaNEW->elements,(void*)imprimirIdDTB);
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
					list_iterate(colaREADY->elements,(void*)imprimirIdDTB);
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
					list_iterate(colaBLOCK->elements,(void*)imprimirIdDTB);
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
					list_iterate(colaEXEC->elements,(void*)imprimirIdDTB);
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
					list_iterate(colaEXIT->elements,(void*)imprimirIdDTB);
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
									printf("El id %d es un id erroneo, no se encuentra en ninguna cola\n",id);
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
									printf("El id %d es un id erroneo, no se encuentra en ninguna cola\n",id);
								} else {
									printf("El DTB con id %d ya se encuentra en la cola EXIT.\n",id);
								}
							} else {
								printf("El DTB con ID %d esta ejecutando cuando termine se finalizara \n", id);
								list_add(listaProcesosAFinalizar,id);
							}
						} else {
							printf("Se mando el DTB con ID %d a la cola de EXIT\n",id);
							int indice;
							indice = buscarIndicePorIdGDT(colaBLOCK->elements,id);
							miDTB = list_remove(colaBLOCK->elements, indice);
							finalizarDTB(miDTB);
					}
				}else {
					printf("Se mando el DTB con ID %d a la cola de EXIT\n",id);
					int indice;
					indice = buscarIndicePorIdGDT(colaREADY->elements,id);
					miDTB = list_remove(colaREADY->elements, indice);
					finalizarDTB(miDTB);
				}
			} else {
				printf("Se mando el DTB con ID %d a la cola de EXIT\n",id);
				int indice;
				indice = buscarIndicePorIdGDT(colaNEW->elements,id);
				miDTB = list_remove(colaNEW->elements, indice);
				actualizarMetricaEXIT(miDTB->ID_GDT);
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
			if(conectionDAM==true && !list_is_empty(listaCPU)){
				myPuts("El proceso S-AFA esta en un estado OPERATIVO\n");
			}else{
				myPuts("El proceso S-AFA esta en un estado CORRUPTO\n");
			}
		}

		if(!strncmp(linea,"exit",4))
		{
			/*t_queue *colaNEW;
			t_queue *colaREADY;
			t_queue *colaEXEC;
			t_queue *colaBLOCK;
			t_queue *colaEXIT;
			t_queue *colaVRR;*/
			queue_clean_and_destroy_elements(colaNEW, (void*)dtb_destroy);
			exit(1);
		}

		if(!strncmp(linea,"cpus",4))
		{
			mostrarCPUs();
		}

		free(linea);
	}
	return EXIT_SUCCESS;
}
