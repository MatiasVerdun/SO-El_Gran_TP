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

//#define PATHCONFIGSAFA "/home/utnso/tp-2018-2c-smlc/Config/S-AFACompleta.txt"
#define PATHCONFIGSAFA "/home/utnso/tp-2018-2c-smlc/Config/S-AFA.txt"


//t_config *configSAFA;

bool operacionDummyOK = true;
int IDGlobal = 0;
int DTBenPCP = 0;
int estadoSistema = -1; // 0 = Estado Operativo
int cantHistoricaDeSentencias = 0;
int cantDeSentenciasQueUsaronADiego = 0;
int cantDeSentenciasHastaExit;
int cantDTB;
int GsocketDAM;

int tiempoCPUs;
int tiempoSAFAI;
int tiempoSAFAF;

enum{ COLA_NEW,
	  COLA_EXEC,
	  COLA_EXIT,
	  COLA_READY,
	  COLA_BLOCK,
	  COLA_VRR };

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
	int sentenciasEjecutadas;
	int sentenciasEjecutadasEnNEW;
	int sentenciasEjecutadasHastaEXIT;
	//int sentenciasEjecutadasQueFueronAlDiego;
} metrica;

typedef struct DTBPrioridadVRR{
	DTB* unDTB;
	int remanente;
	int bloqueado; // 1 si o 0 no
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


t_list *listaMetricas;
t_list *listaCPU;
t_list *listaProcesosAFinalizar;
t_list *listaRecursos;

static sem_t semPlani;
static sem_t semDAM;
static bool conectionDAM = false;

	///PROTOTIPOS///
void verificarSiSePuedeLiberarUnRecurso(recurso *miRecurso);
void finalizarDTB(bool post,int idDTB, t_list* miLista);
void operacionDummy();
int buscarIndiceListaDeMetricas(t_list* miLista, int idDTB);

	///FUNCIONES DE CONFIG///

void mostrarConfig(){

	char* myText = string_from_format("DAM -> IP: %s - Puerto: %d \0", configModificable.IP_ESCUCHA,configModificable.PUERTO_DAM);
	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);

	myText = string_from_format("CPU -> IP: %s - Puerto: %d \0", configModificable.IP_ESCUCHA,configModificable.PUERTO_CPU);
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);

	myText = string_from_format("Algoritmo: %s \0", configModificable.algoPlani);
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);

	myText = string_from_format("Grado de multiprogramacion: %d \0", configModificable.gradoMultiprogramacion);
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);

	myText = string_from_format("Quantum: %d \0", configModificable.quantum);
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);

	myText = string_from_format("Retardo: %d \0" COLOR_RESET , configModificable.retardoPlani);
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
}

void actualizarConfig(bool consola){
	t_config *configAux= config_create(PATHCONFIGSAFA);
	bool mostrar = false;

	if(strcmp(configModificable.algoPlani,config_get_string_value(configAux,"ALGO_PLANI"))!=0){
		strcpy(configModificable.algoPlani,config_get_string_value(configAux,"ALGO_PLANI"));
		mostrar = true;
	}

	if(configModificable.quantum!=config_get_int_value(configAux,"Q")){
		configModificable.quantum=config_get_int_value(configAux,"Q");
		mostrar= true;
	}

	if(configModificable.gradoMultiprogramacion!=config_get_int_value(configAux,"GMP")){
		configModificable.gradoMultiprogramacion=config_get_int_value(configAux,"GMP");
		mostrar=true;
	}

	if(configModificable.retardoPlani!=config_get_int_value(configAux,"RETARDO")){
		configModificable.retardoPlani=config_get_int_value(configAux,"RETARDO");
		mostrar = true;
	}

	if(mostrar && !consola){
		myPuts(MAGENTA"------ La configuracion cambio, la nueva configuracion es: ------ \n");
		mostrarConfig();
		printf("\n");
	}

	if(consola){
		mostrarConfig();
		printf("\n");
	}

	config_destroy(configAux);

}

void cargarConfig(){
	t_config *configSAFA= config_create(PATHCONFIGSAFA);

	strcpy(configModificable.IP_ESCUCHA,config_get_string_value(configSAFA,"IP_ESCUCHA"));
	strcpy(configModificable.algoPlani,config_get_string_value(configSAFA,"ALGO_PLANI"));
	configModificable.PUERTO_DAM=config_get_int_value(configSAFA,"DAM_PUERTO");
	configModificable.PUERTO_CPU=config_get_int_value(configSAFA,"CPU_PUERTO");
	configModificable.retardoPlani=config_get_int_value(configSAFA,"RETARDO");
	configModificable.quantum=config_get_int_value(configSAFA,"Q");
	configModificable.gradoMultiprogramacion=config_get_int_value(configSAFA,"GMP");

	config_destroy(configSAFA);


}

	//FUNCIONES PARA LA LISTAS/COLAS
	///CREAR STRUCTS///
static void dtb_destroy(DTB* miDTB){
	free(miDTB->tablaArchivosAbiertos);

	free(miDTB);
}

DTB* crearDTB(char *rutaMiScript){
	DTB *miDTB; //Sin free por que sino cuando lo meto en la cola pierdo el elemento
	miDTB = malloc(sizeof(DTB));

	miDTB->ID_GDT = IDGlobal;
	strcpy(miDTB->Escriptorio,rutaMiScript);
	miDTB->PC = 0;
	miDTB->Flag_GDTInicializado = 1;
	miDTB->tablaArchivosAbiertos = list_create();
	miDTB->totalDeSentenciasAEjecutar = 0;

	IDGlobal++;

	return miDTB;
}

metrica* crearMetricasParaDTB(int IDDTB){
	metrica *miMetrica;
	miMetrica = malloc(sizeof(metrica));

	miMetrica->ID_DTB = IDDTB;
	miMetrica->sentenciasEjecutadas = 0;
	miMetrica->sentenciasEjecutadasEnNEW = cantHistoricaDeSentencias;
	miMetrica->sentenciasEjecutadasHastaEXIT= cantHistoricaDeSentencias;
	//miMetrica->sentenciasEjecutadasQueFueronAlDiego = 0;

	return miMetrica;
}

DTBPrioridadVRR *crearDTBVRR(DTB* miDTB, int sobra){
	DTBPrioridadVRR *miDTBVRR;
	miDTBVRR = malloc(sizeof(DTBPrioridadVRR));

	miDTBVRR->unDTB = miDTB;
	miDTBVRR->remanente = sobra;
	miDTBVRR->bloqueado = 1;

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
	recurso *miRecurso = malloc(sizeof(recurso));

	int largoRecurso =  strlen(strRecurso);
	miRecurso->recurso = malloc(largoRecurso + 1);
	strcpy(miRecurso->recurso,strRecurso);

	miRecurso->semaforo = 1;
	miRecurso->DTBWait = list_create();
	miRecurso->DTBBloqueados = list_create();

	return miRecurso;
}

datosArchivo* crearDatosArchivos(char* pathArchivo,int fileID){
	datosArchivo *miArchivo;
	miArchivo = malloc(sizeof(datosArchivo));

	strcpy(miArchivo->pathArchivo,pathArchivo);

	miArchivo->fileID = fileID;

	return miArchivo;
}

	///CPU's///

bool hayCPUDisponible(){
	for(int i = 0; i < list_size(listaCPU); i++){
		clienteCPU * elemento;

		elemento = list_get(listaCPU,i);

		if(elemento->libre == 1)
		return true;
	}
return false;
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

int  buscarIndicePorSockCPU(int sockCPU){
	for(int i = 0 ; i < list_size(listaCPU); i++){

		clienteCPU * elemento;

		elemento = list_get(listaCPU,i);

		if(elemento->socketCPU == sockCPU)
			return i;
	}

	return -1;
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

int buscarCPUDesconectada(){
	int chequear = PREGUNTAR_DESCONEXION_CPU;

	for(int i = 0; i < list_size(listaCPU); i++){
		clienteCPU * cpu = list_get(listaCPU,i);

		myEnviarDatosFijos(cpu->socketCPU,&chequear,sizeof(int));

		if(myRecibirDatosFijos(cpu->socketCPU,&chequear,sizeof(int))==1){
			return cpu->socketCPU;
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

	///DTB's///

DTB* buscarDTBPorID(t_queue *miCola,int idGDT){
	for (int indice = 0;indice < list_size(miCola->elements);indice++){
			DTB *miDTB= list_get(miCola->elements,indice);
			if(miDTB->ID_GDT==idGDT){
				return miDTB;
			}
	}
	return NULL;
}

int buscarIndicePorIdGDT(t_list* miLista,int idGDT){
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

int esIDDTBValido(int idDTBABuscar){

	int indice = buscarIndiceListaDeMetricas(colaEXIT->elements,idDTBABuscar);
	if(indice != -1){
		return COLA_EXIT;
	}

	indice = buscarIndicePorIdGDT(colaBLOCK->elements,idDTBABuscar);
	if(indice != -1){
		return COLA_BLOCK;
	}

	indice = buscarIndicePorIdGDT(colaREADY->elements,idDTBABuscar);
	if(indice != -1){
		return COLA_READY;
	}

	indice = buscarIndicePorIdGDT(colaEXEC->elements,idDTBABuscar);
	if(indice != -1){
		return COLA_EXEC;
	}

	indice = buscarIndicePorIdGDT(colaNEW->elements,idDTBABuscar);
	if(indice != -1){
		return COLA_NEW;
	}

	indice = buscarIndicePorIdGDT(colaVRR->elements,idDTBABuscar);
	if(indice != -1){
		return COLA_VRR;
	}

	return -1;
}

void copiarDatosDTB(DTB *miDTB,DTB *DTBrecibido){

	miDTB->PC = DTBrecibido->PC;

	miDTB->totalDeSentenciasAEjecutar = DTBrecibido->totalDeSentenciasAEjecutar;

	//list_destroy_and_destroy_elements(miDTB->tablaArchivosAbiertos, (void*)free);

	list_clean_and_destroy_elements(miDTB->tablaArchivosAbiertos, (void*)free);

	list_add_all(miDTB->tablaArchivosAbiertos, DTBrecibido->tablaArchivosAbiertos);
	//miDTB->tablaArchivosAbiertos = DTBrecibido->tablaArchivosAbiertos;

}

bool hayPrioridad(){
	int estado;
	DTBPrioridadVRR *miDTB;
	if(!queue_is_empty(colaVRR)){
		for (int indice = 0;indice < queue_size(colaVRR);indice++){
			miDTB = list_get(colaVRR->elements,indice);
			estado = miDTB->bloqueado;
			if(estado == 0)
				return true;
		}
	}
	return false;

}

DTBPrioridadVRR* elegirVRRDesbloqueado(){
	DTBPrioridadVRR *miDTBVRR;
	for (int indice = 0;indice < queue_size(colaVRR);indice++){
		miDTBVRR= list_get(colaVRR->elements,indice);
		if(miDTBVRR->bloqueado==0){
			return miDTBVRR = list_remove(colaVRR->elements,indice);
		}
	}
	return NULL;
}

DTBPrioridadVRR* buscarDTBVRRPorID(int idDTB){
	DTBPrioridadVRR *miDTBVRR;
	for (int indice = 0;indice < queue_size(colaVRR);indice++){
			miDTBVRR= list_get(colaVRR->elements,indice);
			if(miDTBVRR->unDTB->ID_GDT==idDTB){
					return miDTBVRR;
			}
	}
	return NULL;
}

int buscarIndiceDTBVRR(int idDTB){
	DTBPrioridadVRR *miDTBVRR;
	for (int indice = 0;indice < queue_size(colaVRR);indice++){
			miDTBVRR= list_get(colaVRR->elements,indice);
			if(miDTBVRR->unDTB->ID_GDT==idDTB){
					return indice;
			}
	}
	return -1;
}

	///RECURSOS///
int buscarIndicePorRecurso(char* recursoABuscar){
	for (int indice = 0;indice < list_size(listaRecursos);indice++){
		recurso *miRecurso= list_get(listaRecursos,indice);
		if(strcmp(miRecurso->recurso,recursoABuscar)==0){
			return indice;
		}
	}
	return -1;
}

void mostrarRecursos(){
	for(int i = 0; i < list_size(listaRecursos); i++){
		recurso * miRecurso = list_get(listaRecursos,i);

		myPuts(BLUE"Recurso: '%s'"COLOR_RESET"\n",miRecurso->recurso);
		myPuts(MAGENTA"Semaforo: %d"COLOR_RESET"\n",miRecurso->semaforo);
		myPuts(MAGENTA"DTB ASIGNADOS: "COLOR_RESET"\n");
		if(list_is_empty(miRecurso->DTBWait)){
			myPuts(BOLDMAGENTA"No hay DTB asignados"COLOR_RESET"\n");
		}else{
			for(int j = 0; j < list_size(miRecurso->DTBWait); j++){
				myPuts(BOLDMAGENTA"DTB NRO: %d"COLOR_RESET"\n",list_get(miRecurso->DTBWait,j));
			}
		}
		myPuts(MAGENTA"DTB BLOQUEADOS: "COLOR_RESET"\n");
		if(list_is_empty(miRecurso->DTBBloqueados)){
			myPuts(BOLDMAGENTA"No hay DTB bloqueados"COLOR_RESET"\n");
		}else{
			for(int k = 0; k < list_size(miRecurso->DTBBloqueados); k++){
				myPuts(BOLDMAGENTA"DTB NRO: %d "COLOR_RESET"\n",list_get(miRecurso->DTBBloqueados,k));
			}
		}
	}
}

int buscarIndiceListaDeIDDTB(t_list *miLista, int idDTBBuscado){
	for (int indice = 0;indice < list_size(miLista);indice++){

		int idDTB =(int) list_get(miLista,indice);
		if(idDTB == idDTBBuscado){
				return indice;
		}
	}
	return -1;
}

	///FUNCIONES METRICAS///

int buscarIndiceListaDeMetricas(t_list* miLista, int idDTB){	//La cola EXIT es una lista de Metricas
	for (int indice = 0;indice < list_size(miLista);indice++){
			metrica *miMetrica= list_get(miLista,indice);
			if(miMetrica->ID_DTB==idDTB){
				return indice;
			}
		}
		return -1;
}

metrica* buscarMetrica(int idDTB){
	metrica* laMetrica;

	int indice = buscarIndiceListaDeMetricas(colaEXIT->elements,idDTB);
	if(indice != -1){

		laMetrica = list_get(colaEXIT->elements,indice);

	}else{
		bool esMetricaDelDTB(metrica *metricaAux){
			return metricaAux->ID_DTB == idDTB;
		}

		laMetrica = list_find(listaMetricas, (void*) esMetricaDelDTB);
	}
	return laMetrica;
}


metrica* actualizarMetricaEXIT(int idDTB){

	int indice  = buscarIndiceListaDeMetricas(listaMetricas,idDTB);

	metrica* laMetrica = list_remove(listaMetricas,indice);

	laMetrica->sentenciasEjecutadasHastaEXIT= cantHistoricaDeSentencias - laMetrica->sentenciasEjecutadasHastaEXIT;

	cantDeSentenciasHastaExit = cantDeSentenciasHastaExit + laMetrica->sentenciasEjecutadasHastaEXIT;

	return laMetrica;
}

void actualizarMetricaNEW(int idDTB){

	bool esMetricaDelDTB(metrica *metricaAux){
		return metricaAux->ID_DTB == idDTB;
	}

	metrica *laMetrica;
	laMetrica = list_find(listaMetricas, (void*) esMetricaDelDTB);

	laMetrica->sentenciasEjecutadasEnNEW = cantHistoricaDeSentencias - 	laMetrica->sentenciasEjecutadasEnNEW;
}

/*
void actualizarMetricaDiego(int idDTB){

	bool esMetricaDelDTB(metrica *metricaAux){
		return metricaAux->ID_DTB == idDTB;
	}

	metrica *laMetrica;
	laMetrica = list_find(listaMetricas, (void*) esMetricaDelDTB);

	laMetrica->sentenciasEjecutadasQueFueronAlDiego += 1;
}*/

void actualizarMetricaSentenciasEjecutadas(int idDTB, int instrucciones){

	bool esMetricaDelDTB(metrica *metricaAux){
		return metricaAux->ID_DTB == idDTB;
	}

	metrica *laMetrica;
	laMetrica = list_find(listaMetricas, (void*) esMetricaDelDTB);

	laMetrica->sentenciasEjecutadas  += instrucciones;
}

void mostrarMetricasDTB(int idDTB){
	metrica *laMetrica;

	if(esIDDTBValido(idDTB)!=-1){

		laMetrica = buscarMetrica(idDTB);


		myPuts(BLUE "Las Metricas del DTB %d son :"COLOR_RESET"\n", idDTB);
		myPuts(MAGENTA "Cant. de sentencias ejecutadas que esperó un DTB en la cola NEW: %d"COLOR_RESET"\n",laMetrica->sentenciasEjecutadasEnNEW);

	}else{
		myPuts(RED "El ID: %d no es valido"COLOR_RESET"\n", idDTB);
	}
}

void mostrarMetricasSistema(){
	float promedioDiego;
	float promedioEXIT;
	float tiempoPromedio;
	float tiempoSAFA;
	float porcentaje;

	promedioEXIT  = cantDeSentenciasHastaExit/(float)cantDTB;
	promedioDiego = cantDeSentenciasQueUsaronADiego/(float)cantDTB;
	tiempoSAFAF = time(NULL);
	tiempoSAFA = tiempoSAFAF-tiempoSAFAI;
	tiempoPromedio = tiempoCPUs / tiempoSAFA;
	porcentaje = ( cantDeSentenciasQueUsaronADiego * 100) /(float)cantHistoricaDeSentencias;

	myPuts(BLUE "Las Metricas del Sistema son :" COLOR_RESET"\n");
	myPuts(MAGENTA "Cant.de sentencias ejecutadas prom. del sistema que usaron a El Diego:  %f"COLOR_RESET"\n",promedioDiego);
	myPuts(MAGENTA "Cant. de sentencias ejecutadas prom. del sistema para que un DTB termine en la cola EXIT:  %f "COLOR_RESET"\n",promedioEXIT);
	myPuts(MAGENTA "Porcentaje de las sentencias ejecutadas promedio que fueron a “El Diego”:  %f %%"COLOR_RESET"\n",porcentaje);
	myPuts(MAGENTA "Tiempo de Respuesta promedio del Sistema %f "COLOR_RESET"\n",tiempoPromedio);
}


	///PLANIFICACION A LARGO PLAZO///

void PLP(){

	while(!queue_is_empty(colaNEW) ){

		actualizarConfig(false);

		if (DTBenPCP <= configModificable.gradoMultiprogramacion && operacionDummyOK){

			operacionDummy();
		}
	}
}

void NuevoDTByPlanificacion(char *rutaScript){
	DTB *miDTB;
	metrica *laMetrica;

	miDTB = crearDTB(rutaScript);

	queue_push(colaNEW,miDTB);

	myPuts(BLUE "El DTB NRO  %d esta en la cola NEW" COLOR_RESET "\n",miDTB->ID_GDT);

	laMetrica = crearMetricasParaDTB(miDTB->ID_GDT);
	list_add(listaMetricas,laMetrica);

	cantDTB ++;

	PLP();
}


	///ACCION SEGUN PLANIFICACION ///

void accionSegunPlanificacion(int socketCPU,DTB* DTBrecibido, int motivoLiberacionCPU, int instruccionesRealizadas){
	int indice;
	int sobra;
	int motError;

	indice=buscarIndicePorIdGDT(colaEXEC->elements,DTBrecibido->ID_GDT);
	DTB *miDTB = list_remove(colaEXEC->elements, indice);

	copiarDatosDTB(miDTB,DTBrecibido);

	switch(motivoLiberacionCPU){

	case MOT_QUANTUM:
		queue_push(colaREADY,miDTB);

		myPuts(BLUE "El DTB NRO  %d esta en la cola READY" COLOR_RESET "\n",miDTB->ID_GDT);

	break;

	case MOT_BLOQUEO:

		if(miDTB->Flag_GDTInicializado == 1){		//Los DUMMY no cuentan para las metricas
			cantDeSentenciasQueUsaronADiego ++; // Todas las sentencias que se bloquearon usaron al DAM y solo se puede hacer de a una por vez
			//actualizarMetricaDiego(miDTB->ID_GDT);
		}

		if(strcmp(configModificable.algoPlani,"FIFO")==0 || strcmp(configModificable.algoPlani,"RR")==0){
			queue_push(colaBLOCK,miDTB);

			myPuts(BLUE "El DTB NRO  %d esta en la cola BLOCK" COLOR_RESET "\n",miDTB->ID_GDT);
		}

		if(strcmp(configModificable.algoPlani,"VRR") == 0){
			sobra = configModificable.quantum - instruccionesRealizadas;

			if(sobra > 0 && miDTB->Flag_GDTInicializado == 1){

				DTBPrioridadVRR * miDTBVRR;
				miDTBVRR = crearDTBVRR(miDTB,sobra);

				queue_push(colaVRR,miDTBVRR);
				myPuts(BLUE "El DTB NRO  %d esta en la cola DE PRIORIDAD VRR" COLOR_RESET "\n",miDTB->ID_GDT);

			}else{

				queue_push(colaBLOCK,miDTB);

				myPuts(BLUE "El DTB NRO  %d esta en la cola BLOCK" COLOR_RESET "\n",miDTB->ID_GDT);
			}
		}
	break;

	case MOT_FINALIZO:
		queue_push(colaEXEC,miDTB);

		myPuts(BOLDGREEN"El DTB NRO: %d finalizo su ejecucion"COLOR_RESET"\n",miDTB->ID_GDT);

		finalizarDTB(false,miDTB->ID_GDT,colaEXEC->elements);

	break;

	case MOT_ERROR:
		queue_push(colaEXEC,miDTB);
		myRecibirDatosFijos(socketCPU,&motError,sizeof(int));

		switch(motError){
		case 20001:
			myPuts(RED"Hubo un error en la operacion ASIGNAR (el archivo no se encuentra abierto) se aborto el  DTB NRO: %d "COLOR_RESET"\n",miDTB->ID_GDT);
		break;
		case 20002:
			myPuts(RED"Hubo un error en la operacion ASIGNAR (fallo segmento/memoria) se aborto el  DTB NRO: %d "COLOR_RESET"\n",miDTB->ID_GDT);
		break;
		case 20003:
			myPuts(RED"Hubo un error en la operacion ASIGNAR (espacio insuficiente FM9) se aborto el  DTB NRO: %d "COLOR_RESET"\n",miDTB->ID_GDT);
		break;
		case 40001:
			myPuts(RED"Hubo un error en la operacion CLOSE (el archivo no se encuentra abierto) se aborto el  DTB NRO: %d "COLOR_RESET"\n",miDTB->ID_GDT);
		break;

		}

		finalizarDTB(false,miDTB->ID_GDT,colaEXEC->elements);

	break;

	}

}

	///ENVIAR Y RECIBIR MENSAJES PARA LA PLANIFICACION///

void enviarQyPlanificacionACPU(int CPU,int remanente){
	char *planificacion=malloc(5);
	memset(planificacion,'\0',5);
	int quantum = configModificable.quantum;

	if(strcmp(configModificable.algoPlani,"FIFO")==0){
		quantum = -1;
	}
	memcpy(planificacion,configModificable.algoPlani,strlen(configModificable.algoPlani));

	myEnviarDatosFijos(CPU,planificacion,5);

	if(strcmp(configModificable.algoPlani,"VRR")==0){
		myEnviarDatosFijos(CPU,&remanente,sizeof(int));
	}

	myEnviarDatosFijos(CPU,&quantum,sizeof(int));
	free(planificacion);
}

void recibirDTBeInstrucciones(int socketCPU,int motivoLiberacionCPU){
	DTB *miDTBrecibido;
	clienteCPU *miCPU;
	int instruccionesRealizadas;
	int tiempoCPU = 0;

	miDTBrecibido = recibirDTB(socketCPU);

	myRecibirDatosFijos(socketCPU,&instruccionesRealizadas,sizeof(int));

	if(miDTBrecibido->Flag_GDTInicializado ==1){
		myRecibirDatosFijos(socketCPU,&tiempoCPU,sizeof(int));
		actualizarMetricaSentenciasEjecutadas(miDTBrecibido->ID_GDT,instruccionesRealizadas);
		tiempoCPUs  = tiempoCPUs + tiempoCPU;
		cantHistoricaDeSentencias += instruccionesRealizadas;
	}

	miCPU = buscarCPUporSock(socketCPU);	//Se libera la CPU
	miCPU->libre = 1;

	if(!list_is_empty(listaProcesosAFinalizar)){
		if(debeFinalizarse(miDTBrecibido->ID_GDT)){
			int indice;
			indice = buscarIndiceListaDeIDDTB(listaProcesosAFinalizar,miDTBrecibido->ID_GDT);
			list_remove_and_destroy_element(listaProcesosAFinalizar, indice,(void*)free);

			myPuts(GREEN "Se mando correctamente el DTB con ID %d a la cola de EXIT por el comando finalizar"COLOR_RESET"\n",miDTBrecibido->ID_GDT);

			finalizarDTB(false,miDTBrecibido->ID_GDT,colaEXEC->elements);

		}
	}else{
		accionSegunPlanificacion(socketCPU,miDTBrecibido,motivoLiberacionCPU,instruccionesRealizadas);
	}


	list_destroy(miDTBrecibido->tablaArchivosAbiertos);

	free(miDTBrecibido);


}

	///PLANIFICACION CORTO PLAZO///

void asignarRecurso(int socketCPU, recurso *miRecurso, int idDTB){
	int resultado;

	if(miRecurso->semaforo >= 0 ){
		list_add(miRecurso->DTBWait,(int*)idDTB);
		resultado = 1;
		myPuts(GREEN"Se asigno el recurso '%s' al DTB %d, enviando confirmacion"COLOR_RESET"\n",miRecurso->recurso,idDTB);
		myEnviarDatosFijos(socketCPU,&resultado,sizeof(int)); // OK
	}else{
		list_add(miRecurso->DTBBloqueados,(int*)idDTB);
		resultado = 0;
		myPuts(MAGENTA"No se pudo asignar el recurso '%s' se bloqueara el DTB %d, avisando a CPU "COLOR_RESET"\n",miRecurso->recurso,idDTB);
		myEnviarDatosFijos(socketCPU,&resultado,sizeof(int)); // BloquearDTB
	}

}

char* recibirDatosAccionWaitSignal(int socketCPU){
	char * recurso;
	int largoRecurso;

	if(myRecibirDatosFijos(socketCPU,&largoRecurso,sizeof(int))==1)
		myPuts(RED"Error al recibir el tamaño del recurso"COLOR_RESET"\n");

	recurso = malloc(largoRecurso+1);
	memset(recurso,'\0',largoRecurso+1);

	if(myRecibirDatosFijos(socketCPU,recurso,largoRecurso)==1)
		myPuts(RED"Error al recibir el recurso"COLOR_RESET"\n");

	return recurso;
}

void ejecutarAccionWaitSignal(int accion, int socketCPU,DTB *miDTB){
	recurso *miRecurso;
	int indiceRecurso;
	int indiceEXEC;
	char * recurso = NULL;
	int continuarEjecucion;

	switch(accion){
	case ACC_WAIT:
		recurso = recibirDatosAccionWaitSignal(socketCPU);
		indiceEXEC  = buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
		if(indiceEXEC  != -1){
			myPuts(BLUE"Ejecutando OPERACION WAIT");
			loading(1);

			indiceRecurso = buscarIndicePorRecurso(recurso);

			if(indiceRecurso == -1){
				miRecurso = crearRecurso(recurso);

				list_add(listaRecursos,miRecurso);

				myPuts(GREEN"Se creo el recurso '%s' , enviando confirmacion"COLOR_RESET"\n",miRecurso->recurso);

				miRecurso->semaforo -= 1;

				continuarEjecucion = 1;
				myEnviarDatosFijos(socketCPU,&continuarEjecucion,sizeof(int)); // OK
			}else{
				miRecurso = list_get(listaRecursos,indiceRecurso);

				myPuts(GREEN"Se hizo WAIT del recurso '%s' , enviando confirmacion"COLOR_RESET"\n",miRecurso->recurso);

				miRecurso->semaforo -= 1;

				asignarRecurso(socketCPU,miRecurso,miDTB->ID_GDT);
			}
		}else{
			continuarEjecucion = 0;
			myEnviarDatosFijos(socketCPU,&continuarEjecucion,sizeof(int)); // EL DTB FINALIZO (POR CONSOLA)
		}
	break;

	case ACC_SIGNAL:
		recurso = recibirDatosAccionWaitSignal(socketCPU);
		indiceEXEC  = buscarIndicePorIdGDT(colaEXEC->elements,miDTB->ID_GDT);
		if(indiceEXEC != -1){
			myPuts(BLUE"Ejecutando OPERACION SIGNAL");
			loading(1);

			continuarEjecucion = 1;
			myEnviarDatosFijos(socketCPU,&continuarEjecucion,sizeof(int)); // OK

			indiceRecurso = buscarIndicePorRecurso(recurso);

			if(indiceRecurso == -1){
				miRecurso = crearRecurso(recurso);
				list_add(listaRecursos,miRecurso);

				myPuts(GREEN"Se creo el recurso '%s' , enviando confirmacion"COLOR_RESET"\n",miRecurso->recurso);

			}else{
				miRecurso = list_get(listaRecursos,indiceRecurso);
				myPuts(GREEN"Se hizo SIGNAL del recurso '%s' , enviando confirmacion"COLOR_RESET"\n",miRecurso->recurso);
				miRecurso->semaforo += 1;
				verificarSiSePuedeLiberarUnRecurso(miRecurso);
			}
		}else{
			continuarEjecucion = 0;
			myEnviarDatosFijos(socketCPU,&continuarEjecucion,sizeof(int)); // EL DTB FINALIZO (POR CONSOLA)
		}
	break;

	}
	free(recurso);
}

DTB * elegirProximoAEjecutarSegunPlanificacion(int socketCPU, int *remanente){

	DTB* miDTB;
	if(strcmp(configModificable.algoPlani,"FIFO")==0|| strcmp(configModificable.algoPlani,"RR")==0){

		miDTB = queue_pop(colaREADY);

	}else if(strcmp(configModificable.algoPlani,"VRR")==0){
		if(!queue_is_empty(colaVRR) && hayPrioridad()){
			DTBPrioridadVRR *DTBVRR;

			DTBVRR = elegirVRRDesbloqueado();

			*remanente= DTBVRR->remanente;

			miDTB = DTBVRR->unDTB;

			//free(DTBVRR); Aca no
		}else{
			*remanente = 0;

			miDTB = queue_pop(colaREADY);
		}
	}
	return miDTB;
}

void PCP(){

	/*Este desbloqueo se efectuará indicando al DTB_Dummy en su contenido un flag de inicialización en
		0, el ID del DTB a “pasar” a “READY” y el path del script Escriptorio a cargar a memoria.*/
	while(hayCPUDisponible() && (!queue_is_empty(colaREADY) || hayPrioridad() ) ) {
		char* strDTB;
		int remanenteVRR=-1;
		int motivo;
		int ejecucion = EJECUCION_NORMAL;

		clienteCPU*CPULibre;

		CPULibre = buscarCPUDisponible();
		int socketCPU = CPULibre->socketCPU;
		CPULibre->libre = 0;                						//PONGO LA CPU COMO OCUPADA

		actualizarConfig(false);

		DTB * miDTB;
		miDTB = elegirProximoAEjecutarSegunPlanificacion(socketCPU,&remanenteVRR);

		myEnviarDatosFijos(socketCPU, &ejecucion, sizeof(int));		//ENVIAR EJECUCION NORMAL

		enviarQyPlanificacionACPU(socketCPU,remanenteVRR);			//ENVIAR PLANIFICACION Y QUANTUM Y RETARDO SI CORRESPONDE

		sleep((int)configModificable.retardoPlani/(float)1000); 				//RETARDO DE PLANIFICACION

		CPULibre->idDTB = miDTB->ID_GDT;

		queue_push(colaEXEC, miDTB);

		strDTB = DTBStruct2String (miDTB);
		myEnviarDatosFijos(socketCPU, strDTB, strlen(strDTB));		//ENVIAR DTB

		if(myRecibirDatosFijos(socketCPU,&motivo,sizeof(int))){
			myPuts(RED "ERROR al recibir el motivo de la liberacion de la CPU %d"COLOR_RESET"\n", socketCPU);
		}

		while(motivo == ACC_SIGNAL || motivo == ACC_WAIT){

			ejecutarAccionWaitSignal(motivo,socketCPU,miDTB);

			if(myRecibirDatosFijos(socketCPU,&motivo,sizeof(int))){
				myPuts(RED "ERROR al recibir el motivo de la liberacion de la CPU %d"COLOR_RESET"\n", socketCPU);
			}
		}

		free(strDTB);

		recibirDTBeInstrucciones(socketCPU,motivo);


	}
}

	///FINALIZAR  Y DESBLOQUEAR DTB///

void desbloquearDTB(int idDTB){
	DTB * miDTB;
	int indice;

	sem_post(&semDAM);

	indice = buscarIndicePorIdGDT(colaBLOCK->elements, idDTB);

	miDTB = list_remove(colaBLOCK->elements,indice);

	queue_push(colaREADY,miDTB);

	myPuts(BLUE "El DTB NRO  %d esta en la cola READY" COLOR_RESET "\n",idDTB);

	sem_post(&semPlani);

}

void verificarSiSePuedeLiberarUnRecurso(recurso *miRecurso){
	if(miRecurso->semaforo >= 0){

		if(!list_is_empty(miRecurso->DTBBloqueados)){

			int idDTBaDesbloquear = (int)list_remove(miRecurso->DTBBloqueados,0); //Lo saca de la cola de Bloqueados y lo pone en la de Wait/Asignado
			list_add(miRecurso->DTBWait, (int*)idDTBaDesbloquear);
			miRecurso->semaforo -= 1;

			myPuts(MAGENTA "El DTB NRO  %d se desbloqueara porque le hicieron SIGNAL a '%s'" COLOR_RESET "\n",idDTBaDesbloquear,miRecurso->recurso);

			desbloquearDTB(idDTBaDesbloquear);

		}
	}
}

void liberarRecursosAsignados(DTB *miDTB){
	if(!list_is_empty(listaRecursos)){
		for(int i = 0 ; i < list_size(listaRecursos); i++){
			recurso * miRecurso = list_get(listaRecursos,i);

			for (int indice = 0;indice < list_size(miRecurso->DTBWait);indice++){ //Se debe hacer asi para que detecte si un DTB hizo mas de un WAIT
				int idDTB =(int) list_get(miRecurso->DTBWait,indice);
				if(idDTB == miDTB->ID_GDT){
					miRecurso->semaforo += 1;
					list_remove(miRecurso->DTBWait,indice);
				}
			}

			verificarSiSePuedeLiberarUnRecurso(miRecurso);

		}
	}
}

void liberarMemoriaFM9(DTB *miDTB){
	int idDTB = miDTB->ID_GDT;
	int accion= OPERACION_CLOSE_ALL;

	myEnviarDatosFijos(GsocketDAM,&accion,sizeof(int));

	myEnviarDatosFijos(GsocketDAM,&idDTB,sizeof(int));

}

void finalizarDTB(bool post,int idDTB, t_list* miLista){
	int indice;
	DTB * miDTB;
	metrica * miMetrica;

	if(post)
		sem_post(&semDAM);

	indice = buscarIndiceListaDeMetricas(colaEXIT->elements,idDTB);
	if(indice == -1){
		indice = buscarIndicePorIdGDT(miLista,idDTB);

		if(indice == -1){
			indice = buscarIndiceDTBVRR(idDTB);
			DTBPrioridadVRR* DTBVRR = list_remove(colaVRR->elements,indice);
			miDTB = DTBVRR->unDTB;
		}else{
			miDTB = list_remove(miLista, indice);
		}


		miMetrica = actualizarMetricaEXIT(miDTB->ID_GDT);

		queue_push(colaEXIT,miMetrica);

		liberarRecursosAsignados(miDTB);

		liberarMemoriaFM9(miDTB);

		list_clean_and_destroy_elements(miDTB->tablaArchivosAbiertos,(void*)free);

		free(miDTB);

		DTBenPCP--;

		PLP();
	}
}

void finalizarPorConsola(int idDTB){

	int cola = esIDDTBValido(idDTB);

	switch(cola){
	case COLA_EXIT:

		myPuts(GREEN "El DTB con id %d ya se encuentra en la cola EXIT" COLOR_RESET "\n",idDTB);

	break;

	case COLA_NEW:

		DTBenPCP++; 	// Porque finalizarDTB hace un -- no se debe hacer aca porque la cola NEW no es PCP
		myPuts(GREEN "Se mando el DTB con ID %d a la cola de EXIT desde la cola NEW" COLOR_RESET "\n",idDTB);
		finalizarDTB(false,idDTB,colaNEW->elements);

	break;

	case COLA_EXEC:

		myPuts(BLUE "El DTB con ID %d esta ejecutando cuando termine se finalizara" COLOR_RESET "\n", idDTB);
		list_add(listaProcesosAFinalizar,(int*)idDTB);

	break;

	case COLA_BLOCK:

		myPuts(GREEN "Se mando el DTB con ID %d a la cola de EXIT desde la cola BLOCK" COLOR_RESET "\n",idDTB);
		finalizarDTB(true,idDTB,colaBLOCK->elements);

	break;


	case COLA_VRR:

		myPuts(GREEN "Se mando el DTB con ID %d a la cola de EXIT desde la cola VRR" COLOR_RESET "\n",idDTB);
		finalizarDTB(false,idDTB,colaVRR->elements);

	break;

	case COLA_READY:

		myPuts(GREEN "Se mando el DTB con ID %d a la cola de EXIT desde la cola READY" COLOR_RESET "\n",idDTB);
		finalizarDTB(false,idDTB,colaREADY->elements);

	break;

	default:
		myPuts(RED "El ID %d no es valido" COLOR_RESET "\n",idDTB);
	break;
	}
}

	///OPERACION DUMMY///

void operacionDummy(){
	if(hayCPUDisponible() ) {
		operacionDummyOK = false;

		char* strDTB;
		clienteCPU*CPULibre;
		DTB* miDTB;
		int ejecucion = EJECUCION_NORMAL;


		miDTB = queue_pop(colaNEW);

		queue_push(colaEXEC,miDTB);

		myPuts(BLUE "El DTB NRO  %d esta en la cola EXEC" COLOR_RESET "\n",miDTB->ID_GDT);

		miDTB->Flag_GDTInicializado = 0; //"Lo transforma en dummy"

		CPULibre = buscarCPUDisponible();
		int socketCPU = CPULibre->socketCPU;
		CPULibre->libre = 0; // Cpu ocupada
		CPULibre->idDTB = miDTB->ID_GDT;

		actualizarConfig(false);

		myEnviarDatosFijos(socketCPU,&ejecucion,sizeof(int));

		enviarQyPlanificacionACPU(socketCPU,0);

		strDTB = DTBStruct2String (miDTB);
		myEnviarDatosFijos(CPULibre->socketCPU, strDTB, strlen(strDTB));

		int motivo;
		myRecibirDatosFijos(socketCPU,&motivo,sizeof(int));

		//sem_post(&semAcceso);

		recibirDTBeInstrucciones(CPULibre->socketCPU,motivo);

		free(strDTB);

		sem_post(&semPlani);

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

	///TABLA DE ARCHIVOS ABIERTOS///

void agregarArchivoALaTabla(DTB* miDTB, char * pathArchivo, int fileID){
	datosArchivo* archivo;

	archivo = crearDatosArchivos(pathArchivo,fileID);

	list_add(miDTB->tablaArchivosAbiertos, archivo);

}

int buscarIndicePorArchivo(char * pathArchivo,t_list* listaArchivos){
	for (int indice = 0;indice < list_size(listaArchivos);indice++){

		datosArchivo *miArchivo= list_get(listaArchivos,indice);

		if(strcmp(miArchivo->pathArchivo,pathArchivo)==0){
			return indice;
		}

	}
	return -1;
}

void borrarArchivoDeLaTabla(DTB* miDTB, char* pathArchivo){
	int indice;
	int fileID;
	int accion;
	int idDTB = miDTB->ID_GDT;
	datosArchivo* miArchivo;

	indice = buscarIndicePorArchivo(pathArchivo,miDTB->tablaArchivosAbiertos);

	if(indice != -1){
		miArchivo = list_get(miDTB->tablaArchivosAbiertos,indice);

		accion = OPERACION_CLOSE;

		myEnviarDatosFijos(GsocketDAM,&accion,sizeof(int));

		myEnviarDatosFijos(GsocketDAM,&idDTB,sizeof(int));

		fileID = miArchivo->fileID;
		myEnviarDatosFijos(GsocketDAM,&fileID,sizeof(int));

		list_remove_and_destroy_element(miDTB->tablaArchivosAbiertos,indice,(void *) free);
	}



}

void verificarSiExisteArchivoEnAlgunaTabla(char *pathArchivo){
	for(int i = 0; i < queue_size(colaBLOCK); i ++){
		DTB *miDTB = list_get(colaBLOCK->elements,i);
		borrarArchivoDeLaTabla(miDTB,pathArchivo);
	}
	for(int i = 0; i < queue_size(colaREADY); i ++){
		DTB *miDTB = list_get(colaREADY->elements,i);
		borrarArchivoDeLaTabla(miDTB,pathArchivo);
	}
	for(int i = 0; i < queue_size(colaEXEC); i ++){
		DTB *miDTB = list_get(colaEXEC->elements,i);
		borrarArchivoDeLaTabla(miDTB,pathArchivo);
	}
	for(int i = 0; i < queue_size(colaVRR); i ++){
		DTBPrioridadVRR *miDTBVRR = list_get(colaVRR->elements,i);
		borrarArchivoDeLaTabla(miDTBVRR->unDTB,pathArchivo);
	}
}

	///FUNCIONES DE INICIALIZACION///

void inicializarSemaforos(){
	sem_init(&semPlani,0,0);
	sem_init(&semDAM,0,1);
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

	indice = buscarIndicePorSockCPU(socketCPU);
	clienteCPU = list_remove(listaCPU,indice);

	if(clienteCPU->libre == 0){
		int idDTB = clienteCPU->idDTB;

		myPuts(RED "Se finalizo el DTB %d porque no se termino de forma correcta la ejecucion" COLOR_RESET "\n", idDTB);

		DTB* miDTB = buscarDTBPorID(colaEXEC,idDTB);

		finalizarDTB(true,miDTB->ID_GDT,colaEXEC->elements); //TODO procesos que usen close asignar wait y signal
	}

	if(list_is_empty(listaCPU)){
		myPuts(RED "No hay CPU's disponibles, no estoy en estado operativo" COLOR_RESET "\n");
		exit(1);
	}

	free(clienteCPU);
}

void gestionarConexionCPU(int* sock){
	int socketCPU = *(int*)sock;

	if(conectionDAM){
		estadoSistema = 0;

		clienteCPU *nuevoClienteCPU;

		nuevoClienteCPU =crearClienteCPU(socketCPU);

		list_add(listaCPU,nuevoClienteCPU);

		if(list_size(listaCPU) == 1){

			myPuts(GREEN"El proceso S-AFA esta en un estado OPERATIVO" COLOR_RESET "\n");
		}

	}else{
			myPuts(RED "Se desconecto el CPU NRO %d" COLOR_RESET "\n", socketCPU);
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

	int result, accion,socketCPUDesconectada, idDTB,tamanio,indice,cantSentencias;
	int fileID = -1;
	char* pathArchivo = NULL ;
	DTB* miDTB = NULL;
	DTBPrioridadVRR* miDTBVRR = NULL;

	while(1){

		result = myRecibirDatosFijos(GsocketDAM,&accion,sizeof(int));
		if(result != 1){

			sem_wait(&semDAM);

			switch(accion){
				case DESCONEXION_CPU:

					socketCPUDesconectada = buscarCPUDesconectada();

					myPuts(RED "Se desconecto la CPU NRO %d" COLOR_RESET "\n", socketCPUDesconectada);

					desconectarCPU(socketCPUDesconectada);


				break;

				case ACC_DUMMY_OK:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					if(myRecibirDatosFijos(GsocketDAM,&fileID,sizeof(int))==1)
						myPuts(RED"Error al recibir el fileID"COLOR_RESET"\n");

					if(myRecibirDatosFijos(GsocketDAM,&cantSentencias,sizeof(int))==1)
						myPuts(RED"Error al recibir la cantidad de sentencias"COLOR_RESET"\n");

					miDTB = buscarDTBPorID(colaBLOCK,idDTB);

					if(miDTB != NULL){
						myPuts(GREEN "EL DTB NRO: %d ejecuto la OPERACION DUMMY correctamente" COLOR_RESET "\n",idDTB);

						operacionDummyOK = true;

						miDTB->Flag_GDTInicializado = 1;

						miDTB->totalDeSentenciasAEjecutar = cantSentencias;

						agregarArchivoALaTabla(miDTB,miDTB->Escriptorio,fileID);

						actualizarMetricaNEW(idDTB);

						DTBenPCP++;

						desbloquearDTB(idDTB);
					}

				break;

				case ACC_DUMMY_ERROR:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(RED"Hubo un error en la operacion DUMMY se aborto el  DTB NRO: %d "COLOR_RESET"\n",idDTB);
					myPuts(">");

					finalizarDTB(true,idDTB, colaBLOCK->elements);

				break;

				case ACC_CREAR_OK:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(GREEN "EL DTB NRO: %d ejecuto la OPERACION CREAR  correctamente" COLOR_RESET "\n",idDTB);

					indice = buscarIndicePorIdGDT(colaBLOCK->elements, idDTB);
					miDTB = list_get(colaBLOCK->elements,indice);

					if(miDTB != NULL ){

						if(miDTB->totalDeSentenciasAEjecutar == miDTB->PC){
							myPuts(BOLDGREEN"El DTB NRO: %d finalizo su ejecucion"COLOR_RESET"\n",miDTB->ID_GDT);

							finalizarDTB(true,idDTB,colaBLOCK->elements);
						}else{
							desbloquearDTB(idDTB);
						}

					}else if(strcmp(configModificable.algoPlani,"VRR") == 0){
						miDTBVRR = buscarDTBVRRPorID(idDTB);

						miDTBVRR->bloqueado = 0;

						sem_post(&semDAM);

						sem_post(&semPlani);
					}

				break;

				case ACC_CREAR_ERROR:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(RED"Hubo un error en la ejecucion CREAR se aborto el  DTB NRO: %d "COLOR_RESET"\n",idDTB);

					finalizarDTB(true,idDTB, colaBLOCK->elements);

				break;

				case ACC_BORRAR_OK:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(GREEN "EL DTB NRO: %d ejecuto la OPERACION BORRAR correctamente" COLOR_RESET "\n",idDTB);

					if(myRecibirDatosFijos(GsocketDAM,&tamanio,sizeof(int))==1)
						myPuts(RED"Error al recibir el tamaño del archivo"COLOR_RESET"\n");

					pathArchivo = malloc(tamanio+1);
					memset(pathArchivo,'\0',tamanio+1);

					if(myRecibirDatosFijos(GsocketDAM,pathArchivo,tamanio)==1)
					myPuts(RED"Error al recibir el path"COLOR_RESET"\n");

					verificarSiExisteArchivoEnAlgunaTabla(pathArchivo);

					indice = buscarIndicePorIdGDT(colaBLOCK->elements, idDTB);
					miDTB = list_get(colaBLOCK->elements,indice);

					if(miDTB != NULL ){
						if(miDTB->totalDeSentenciasAEjecutar == miDTB->PC){
							myPuts(BOLDGREEN"El DTB NRO: %d finalizo su ejecucion"COLOR_RESET"\n",miDTB->ID_GDT);
							finalizarDTB(true,idDTB,colaBLOCK->elements);
						}else{
							desbloquearDTB(idDTB);
						}
					}else if(strcmp(configModificable.algoPlani,"VRR") == 0){
						miDTBVRR = buscarDTBVRRPorID(idDTB);

						miDTBVRR->bloqueado = 0;

						sem_post(&semDAM);

						sem_post(&semPlani);
					}

				break;

				case ACC_BORRAR_ERROR:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(RED"Hubo un error en la ejecucion BORRAR se aborto el  DTB NRO: %d "COLOR_RESET"\n",idDTB);

					finalizarDTB(true,idDTB, colaBLOCK->elements);

				break;

				case ACC_ABRIR_OK:
					if(myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int))==1)
						myPuts(RED"Error al recibir el id del DTB "COLOR_RESET"\n");

					myPuts(GREEN "EL DTB NRO: %d ejecuto la OPERACION ABRIR correctamente" COLOR_RESET "\n",idDTB);

					if(myRecibirDatosFijos(GsocketDAM,&tamanio,sizeof(int))==1)
						myPuts(RED"Error al recibir el tamaño del archivo"COLOR_RESET"\n");

					pathArchivo = malloc(tamanio+1);
					memset(pathArchivo,'\0',tamanio+1);

					if(myRecibirDatosFijos(GsocketDAM,pathArchivo,tamanio)==1)
						myPuts(RED"Error al recibir el path"COLOR_RESET"\n");

					if(myRecibirDatosFijos(GsocketDAM,&fileID,sizeof(int))==1)
						myPuts(RED"Error al recibir el fileID"COLOR_RESET"\n");

					miDTB = buscarDTBPorID(colaBLOCK,idDTB);

					if(miDTB != NULL ){

						agregarArchivoALaTabla(miDTB,pathArchivo,fileID);

						if(miDTB->totalDeSentenciasAEjecutar == miDTB->PC){
							myPuts(BOLDGREEN"El DTB NRO: %d finalizo su ejecucion"COLOR_RESET"\n",miDTB->ID_GDT);
							finalizarDTB(true,idDTB,colaBLOCK->elements);
						}else{
							desbloquearDTB(idDTB);
						}

					}else if(strcmp(configModificable.algoPlani,"VRR") == 0){

						miDTBVRR = buscarDTBVRRPorID(idDTB);

						agregarArchivoALaTabla(miDTBVRR->unDTB,pathArchivo,fileID);

						miDTBVRR->bloqueado = 0;

						sem_post(&semDAM);

						sem_post(&semPlani);
					}

				break;

				case ACC_ABRIR_ERROR:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(RED"Hubo un error en la ejecucion ABRIR  se aborto el  DTB NRO: %d "COLOR_RESET"\n",idDTB);

					finalizarDTB(true,idDTB, colaBLOCK->elements);
				break;

				case ACC_FLUSH_OK:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(GREEN "EL DTB NRO: %d ejecuto la OPERACION FLUSH correctamente" COLOR_RESET "\n",idDTB);

					miDTB = buscarDTBPorID(colaBLOCK,idDTB);

					if(miDTB != NULL ){

						if(miDTB->totalDeSentenciasAEjecutar == miDTB->PC){
							myPuts(BOLDGREEN"El DTB NRO: %d finalizo su ejecucion"COLOR_RESET"\n",idDTB);

							finalizarDTB(true,idDTB,colaBLOCK->elements);
						}else{
							desbloquearDTB(idDTB);
						}

					}else if(strcmp(configModificable.algoPlani,"VRR") == 0){
						miDTBVRR = buscarDTBVRRPorID(idDTB);

						miDTBVRR->bloqueado = 0;

						sem_post(&semDAM);

						sem_post(&semPlani);
					}
				break;

				case ACC_FLUSH_ERROR:
					myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int));

					myPuts(RED"Hubo un error en la ejecucion FLUSH se aborto el  DTB NRO: %d "COLOR_RESET"\n",idDTB);

					finalizarDTB(true,idDTB, colaBLOCK->elements);
				break;

			}

		}else{
			conectionDAM = false;

			myPuts(RED "Se desconecto el proceso DAM" COLOR_RESET "\n");

			exit(1);
		}
	}

}

void* planificacion(){

	while(1){

		actualizarConfig(false);

		sem_wait(&semPlani);

		PCP();

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
	tiempoSAFAI = time(NULL);
	system("clear");
	char *linea;
	pthread_t hiloConnectionCPU; //Nombre de Hilo a crear
	pthread_t hiloConnectionDAM; //Nombre de Hilo a crear
	pthread_t hiloPlanificacion; //Nombre de Hilo a crear

	//configSAFA=config_create(PATHCONFIGSAFA);

	inicializarSemaforos();
	creacionDeColas();
	creacionDeListas();
	cargarConfig();
	mostrarConfig();

	pthread_create(&hiloConnectionDAM,NULL,(void*) &connectionDAM,NULL);
	pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);
	pthread_create(&hiloPlanificacion,NULL,(void*)&planificacion,NULL);


	while (1) {
		actualizarConfig(false);

		linea = readline(">");

		if (linea)
			add_history(linea);

		add_history("ejecutar /scripts/crear_basura.escriptorio");
		add_history("ejecutar /scripts/borrar_complejo.escriptorio");

		add_history("ejecutar /scripts/llena_memoria.escriptorio");

		add_history("ejecutar /scripts/los_borrachos.escriptorio");
		add_history("ejecutar /scripts/la_12.escriptorio");

		add_history("ejecutar /scripts/bloqueo.escriptorio");
		add_history("ejecutar /scripts/inter.escriptorio");

		add_history("ejecutar /scripts/io_bound.escriptorio");
		add_history("ejecutar /scripts/cpu_bound.escriptorio");

		add_history("ejecutar /scripts/complejo.escriptorio");
		add_history("ejecutar /scripts/simple.escriptorio");


		if(!strncmp(linea,"config",6))
		{
			actualizarConfig(true);

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
				printf("La ruta del Escriptorio a ejecutar es: %s \n",path);

				// NuevoDTByPlanificacion(path);
				NuevoDTByPlanificacion(path);

				   free(split[0]);
				   free(split[1]);
				   free(split);
			} else {
				myPuts(RED "El sistema no se encuentra en un estado Operativo, espere a que alcance dicho estado y vuelva a intentar la operacion."COLOR_RESET"\n");
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
					void imprimirMetrica(metrica *miMetrica){
					    myPuts("Posicion: %d ID_GDT: %d\n",indice,miMetrica->ID_DTB);
					    indice++;
					}
					list_iterate(colaEXIT->elements,(void*)imprimirMetrica);
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
			int idDTB = -1;
			split = string_split(linea, " ");

			strcpy(idS,"0");

			strcpy(idS, split[1]);
			idDTB = atoi(idS);

			finalizarPorConsola(idDTB);

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

				mostrarMetricasSistema();

			} else {
				strcpy(idS, split[1]);
				id = atoi(idS);

				mostrarMetricasDTB(id);
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
			//queue_clean_and_destroy_elements(colaEXIT, (void*)dtb_destroy);
			queue_clean_and_destroy_elements(colaEXEC, (void*)dtb_destroy);
			queue_clean_and_destroy_elements(colaREADY, (void*)dtb_destroy);
			queue_clean_and_destroy_elements(colaVRR, (void*)dtb_destroy);
			queue_clean_and_destroy_elements(colaBLOCK, (void*)dtb_destroy);
			queue_clean_and_destroy_elements(colaNEW, (void*)dtb_destroy);
			exit(1);
		}

		if(!strncmp(linea,"cpus",4))
		{
			mostrarCPUs();
		}

		if(!strncmp(linea,"recursos",4))
		{
			mostrarRecursos();
		}

		free(linea);
	}
	return EXIT_SUCCESS;
}
