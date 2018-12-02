#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <console/myConsole.h>
#include <conexiones/mySockets.h>
#include <dtbSerializacion/dtbSerializacion.h>
#include <sentencias/sentencias.h>
#include <parser/parser.h>

#define PATHCONFIGCPU "/home/utnso/tp-2018-2c-smlc/Config/CPU.txt"
t_config *configCPU;

u_int32_t socketGDAM;
u_int32_t socketSAFA;
u_int32_t socketGFM9;

int	nroSentenciaActual;

//Configuracion de Planificacion
int quantum;
int remanente;
int retardoPlanificacion;
char tipoPlanificacion[5];
bool estaBloqueado = false;
int instruccionesEjecutadas = 0; //TODO Capaz que no hace falta que sea global
int motivoLiberacionCPU;
int codigoError = 0;

int tiempoI;
int tiempoF;

static sem_t semDAM;

	///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("DAM   -> IP: %s - Puerto: %s\0",(char*)getConfigR("DAM_IP",0,configCPU), (char*)getConfigR("DAM_PUERTO",0,configCPU) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("S-AFA -> IP: %s - Puerto: %s\0", (char*)getConfigR("S-AFA_IP",0,configCPU), (char*)getConfigR("S-AFA_PUERTO",0,configCPU) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("Retardo: %s\0" , (char*)getConfigR("RETARDO",0,configCPU) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

	///PARSEAR SCRIPTS///

void destruir_operacion(t_parser_operacion op){
	if(op._raw){
		string_iterate_lines(op._raw, (void*) free);
		free(op._raw);
	}
}

sentencia* parsear(char* linea){
	sentencia *laSentencia;

	nroSentenciaActual = -1;

	t_parser_operacion parsed = parse(linea);

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
				printf("No pude interpretar <%s>\n", linea);
				exit(EXIT_FAILURE);
		}
		destruir_operacion(parsed);
		} else {
			fprintf(stderr, "La linea <%s> no es valida\n", linea);
			exit(EXIT_FAILURE);
		}



	if (linea)
		free(linea);

	return laSentencia;
}


	/// VALIDACIONES PLANIFICACION///
bool hayQuantum(int inst){

	if(strcmp(tipoPlanificacion,"VRR")==0 ){
		if(remanente > 0){
			return inst < remanente;
		}else{
			return inst < quantum;
		}
	}

	return inst < quantum || quantum < 0;
}

bool terminoElDTB(DTB *miDTB){
	return miDTB->PC >= miDTB->totalDeSentenciasAEjecutar;
}

bool hayError(){
	return codigoError != 0;
}

bool DTBBloqueado() {
	return estaBloqueado;
}

	/// RECIBIR Y ENVIAR ///

void recibirQyPlanificacion(){

	int resultRecv;

	char bufferPlanificacion[5];
	resultRecv = myRecibirDatosFijos(socketSAFA,bufferPlanificacion,5);
	if(resultRecv !=0){
		printf("Error al recibir el tipo de planificacion");
	}

	strcpy(tipoPlanificacion,bufferPlanificacion);

	if(strcmp(tipoPlanificacion,"VRR")==0){
		resultRecv = myRecibirDatosFijos(socketSAFA,&remanente,sizeof(int));
		if(resultRecv !=0){
				printf("Error al recibir el remanente");
		}
	}

	resultRecv = myRecibirDatosFijos(socketSAFA,&quantum,sizeof(int));
	if(resultRecv !=0){
		printf("Error al recibir el Quantum");
	}


	if(strcmp(tipoPlanificacion,"FIFO")==0){
		printf("-------- Ejecutando segun planificacion FIFO --------\n ");
	}else{
		printf("-------- Ejecutando segun planificacion  %s con Quantum %d -------- \n",tipoPlanificacion,quantum);
	}
}

void recibirRemanente(){
	int resultRecv;

	resultRecv = myRecibirDatosFijos(socketSAFA,&remanente,sizeof(int));

	if(resultRecv !=0){
			printf("Error al recibir el Remanente");
	}

}

void enviarMotivoyDatos(DTB* miDTB, int motivo, int instrucciones, char *recurso){
	myEnviarDatosFijos(socketSAFA,&motivo,sizeof(int));

	if(motivo == ACC_SIGNAL || motivo == ACC_WAIT){
		int tamanio = strlen(recurso);

		myEnviarDatosFijos(socketSAFA,&tamanio,sizeof(int));

		myEnviarDatosFijos(socketSAFA,recurso,tamanio);


	}else{
		char* strDTB;

		strDTB = DTBStruct2String (miDTB);

		myEnviarDatosFijos(socketSAFA,strDTB,strlen(strDTB));

		myEnviarDatosFijos(socketSAFA,&instrucciones,sizeof(int));

		if(miDTB->Flag_GDTInicializado == 1){
			tiempoF = time(NULL);
			int tiempo = tiempoF-tiempoI;
			myEnviarDatosFijos(socketSAFA,&tiempo,sizeof(int));
		}

		free(strDTB);
	}

}

void recibirRespuestaWaitSignal(DTB* miDTB){
	int resultRecv;
	int resultado;

	resultRecv = myRecibirDatosFijos(socketSAFA,&resultado,sizeof(int));
	if(resultRecv ==1){
			myPuts(RED"Error al recibir la respuesta de WAIT/SIGNAL"COLOR_RESET"\n");
	}

	if(resultado == 0){
		estaBloqueado = true;
		motivoLiberacionCPU = MOT_BLOQUEO;
		enviarMotivoyDatos(miDTB,motivoLiberacionCPU,instruccionesEjecutadas,NULL);
	}
}

//

void operacionDummy(DTB *miDTB){
	int largoRuta;
	int operacion;

	int motivo = MOT_BLOQUEO;

	int inst = 0;

	int IDDTB = miDTB->ID_GDT;

	enviarMotivoyDatos(miDTB,motivo,inst,NULL); // Avisar a S-AFA del block y sin instrucciones ejecutadas

	myPuts("Enviando al Diego la ruta del Escriptorio \n");

	largoRuta = strlen(miDTB->Escriptorio);

	operacion = OPERACION_DUMMY;

	myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int));

	myEnviarDatosFijos(socketGDAM,&IDDTB,sizeof(int));

	myEnviarDatosFijos(socketGDAM,&largoRuta,sizeof(int));

	myEnviarDatosFijos(socketGDAM,miDTB->Escriptorio,largoRuta);
}

int buscarFileID(t_list *listaArchivos,char* pathArchivo){
	for (int indice = 0;indice < list_size(listaArchivos);indice++){
		datosArchivo *miArchivo;
		miArchivo = list_get(listaArchivos,indice);
		if(strcmp(miArchivo->pathArchivo,pathArchivo) == 0){
				return miArchivo->fileID;
		}
	}
	return -1;
}

void gestionDeSentencia(DTB *miDTB,sentencia *miSentencia, int instruccionesEjecutadas){
	int fileID;
	int parametro2;
	int operacion = miSentencia->operacion;
	int tamanio;
	int respuestaFM9;
	int idDTB = miDTB->ID_GDT;

	usleep((int)getConfigR("RETARDO",1,configCPU)); //CONCENTRAR

	switch(operacion){

		case OPERACION_ABRIR: //AL DIEGO OP PATH
			fileID = buscarFileID(miDTB->tablaArchivosAbiertos,miSentencia->param1);

			if(fileID == -1){
				estaBloqueado = true;
				motivoLiberacionCPU = MOT_BLOQUEO;
				enviarMotivoyDatos(miDTB,motivoLiberacionCPU,instruccionesEjecutadas,NULL);

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION
				myEnviarDatosFijos(socketGDAM,&idDTB,sizeof(int));			//ENVIO ID DTB

				tamanio = strlen(miSentencia->param1);
				myEnviarDatosFijos(socketGDAM,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO

				myEnviarDatosFijos(socketGDAM,miSentencia->param1,tamanio); //ENVIO EL PATH
			}

		break;

		case OPERACION_CONCENTRAR:
		break;

		case OPERACION_ASIGNAR: // AL FM9 OP ID PARAMETROS
			fileID = buscarFileID(miDTB->tablaArchivosAbiertos,miSentencia->param1);

			if(fileID != -1){

				myEnviarDatosFijos(socketGFM9,&operacion,sizeof(int)); 		//ENVIO OPERACION

				//PARAMETROS

				myEnviarDatosFijos(socketGFM9,&fileID,sizeof(int));

				parametro2= miSentencia->param2;
				myEnviarDatosFijos(socketGFM9,&parametro2,sizeof(int)); //ENVIO LA LINEA

				tamanio = strlen(miSentencia->param3);

				myEnviarDatosFijos(socketGFM9,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO DE LOS DATOS
				myEnviarDatosFijos(socketGFM9,miSentencia->param3,tamanio); //ENVIO DATOS

				myRecibirDatosFijos(socketGFM9,&respuestaFM9,sizeof(int));

				if(respuestaFM9 == 1){
					codigoError = 20002;
				}

				if(respuestaFM9 == 2){
					codigoError = 20003;
				}

			}else{
				codigoError = 20001;										//ERROR: El archivo no se encuentra abierto
			}

		break;

		case OPERACION_WAIT:
			enviarMotivoyDatos(NULL,ACC_WAIT,-1,miSentencia->param1);

			recibirRespuestaWaitSignal(miDTB);
		break;

		case OPERACION_SIGNAL:
			enviarMotivoyDatos(NULL,ACC_SIGNAL,-1,miSentencia->param1);

			recibirRespuestaWaitSignal(miDTB);
		break;

		case OPERACION_FLUSH: // AL DIEGO PARA FM9 OP ID PATH
			fileID = buscarFileID(miDTB->tablaArchivosAbiertos,miSentencia->param1);

			if(fileID != -1){

				estaBloqueado = true;
				motivoLiberacionCPU = MOT_BLOQUEO;
				enviarMotivoyDatos(miDTB,motivoLiberacionCPU,instruccionesEjecutadas,NULL);

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION

				myEnviarDatosFijos(socketGDAM,&idDTB,sizeof(int));			//ENVIO ID DTB

				myEnviarDatosFijos(socketGDAM,&fileID,sizeof(int));			//ENVIO ID

				tamanio = strlen(miSentencia->param1);
				myEnviarDatosFijos(socketGDAM,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO
				myEnviarDatosFijos(socketGDAM,miSentencia->param1,tamanio); //ENVIO EL PATH


			}else{
				codigoError = 30001;										//ERROR: El archivo no se encuentra abierto
			}

		break;

		case OPERACION_CLOSE: // AL FM9 OP ID
			fileID = buscarFileID(miDTB->tablaArchivosAbiertos,miSentencia->param1);

			if(fileID != -1){

				myEnviarDatosFijos(socketGFM9,&operacion,sizeof(int)); 		//ENVIO OPERACION

				myEnviarDatosFijos(socketGFM9,&fileID,sizeof(int));			//ENVIO ID

			}else{
				codigoError = 40001;										//ERROR: El archivo no se encuentra abierto
			}

		break;

		case OPERACION_CREAR:// AL DIEGO PARA MDJ PATH
				estaBloqueado = true;
				motivoLiberacionCPU = MOT_BLOQUEO;
				enviarMotivoyDatos(miDTB,motivoLiberacionCPU,instruccionesEjecutadas,NULL);

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION
				myEnviarDatosFijos(socketGDAM,&idDTB,sizeof(int));			//ENVIO ID DTB

				tamanio = strlen(miSentencia->param1);
				myEnviarDatosFijos(socketGDAM,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO
				myEnviarDatosFijos(socketGDAM,miSentencia->param1,tamanio); //ENVIO EL PATH

				parametro2= miSentencia->param2;
				myEnviarDatosFijos(socketGDAM,&parametro2,sizeof(int)); 	//ENVIO LA CANT DE LINEAS

		break;

		case OPERACION_BORRAR:// AL DIEGO PARA MDJ PATH
				estaBloqueado = true;
				motivoLiberacionCPU = MOT_BLOQUEO;
				enviarMotivoyDatos(miDTB,motivoLiberacionCPU,instruccionesEjecutadas,NULL);

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION
				myEnviarDatosFijos(socketGDAM,&idDTB,sizeof(int));			//ENVIO ID DTB

				tamanio = strlen(miSentencia->param1);
				myEnviarDatosFijos(socketGDAM,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO
				myEnviarDatosFijos(socketGDAM,miSentencia->param1,tamanio); //ENVIO EL PATH

		break;
	}

}

/*
void hardcodearSentencia(){
	//ESTO LO HARIA EL PARSER
	sentencia *laSentencia;

	listaSentencias = list_create();

	laSentencia = malloc(sizeof(sentencia));
	laSentencia->operacion = OPERACION_WAIT;
	laSentencia->param1 = malloc(strlen("HOLA")+1);
	laSentencia->param2 = 1;
	laSentencia->param3 = malloc(4);


	strcpy(laSentencia->param1,"HOLA");

	list_add(listaSentencias,laSentencia);

	sentencia *laSentencia2;

	laSentencia2 = malloc(sizeof(sentencia));
	laSentencia2->operacion = OPERACION_SIGNAL;
	laSentencia2->param1 = malloc(strlen("HOLA")+1);
	laSentencia2->param2 = -1;
	laSentencia2->param3 = malloc(4);

	strcpy(laSentencia2->param1,"HOLA");

	list_add(listaSentencias,laSentencia2);


}*/

void limpiarVariables(sentencia* miSentencia){
	instruccionesEjecutadas = 0;
	motivoLiberacionCPU = -1;
	codigoError = 0;
	estaBloqueado = false;
	free(miSentencia->param1);
	free(miSentencia->param3);
	free(miSentencia);
}

sentencia* buscarSentencia(DTB* miDTB){
	char* laSentencia = NULL;
	sentencia* miSentencia = NULL;
	int PC,operacion,fileEscriptorio;
	int tamanio = 0;

	operacion = OPERACION_LINEA;

	myEnviarDatosFijos(socketGFM9, &operacion, sizeof(int));

	fileEscriptorio = buscarFileID(miDTB->tablaArchivosAbiertos,miDTB->Escriptorio);

	myEnviarDatosFijos(socketGFM9, &fileEscriptorio, sizeof(int));

	PC  = miDTB->PC;
	myEnviarDatosFijos(socketGFM9, &PC, sizeof(int));

	if(myRecibirDatosFijos(socketGFM9,&tamanio,sizeof(int))==1)
		myPuts(RED"Error al recibir el tamaño de la linea"COLOR_RESET"\n");

	laSentencia = malloc(tamanio+1);
	memset(laSentencia,'\0',tamanio+1);

	if(myRecibirDatosFijos(socketGFM9,laSentencia,tamanio)==1)
		myPuts(RED"Error al recibir la sentencia"COLOR_RESET"\n");

	myPuts(MAGENTA"Se ejecutara la siguiente instruccion: %s"COLOR_RESET"\n",laSentencia);

	miSentencia = parsear(laSentencia);

	return miSentencia;
}

void ejecutarInstruccion(DTB* miDTB){
	sentencia *miSentencia;

	while(hayQuantum(instruccionesEjecutadas) && !terminoElDTB(miDTB)  && !hayError() && !DTBBloqueado()){

		miSentencia = buscarSentencia(miDTB);

		miDTB->PC ++;

		instruccionesEjecutadas++;

		gestionDeSentencia(miDTB,miSentencia,instruccionesEjecutadas);

	}

	if(hayError()){
		motivoLiberacionCPU = MOT_ERROR;

	} else if(terminoElDTB(miDTB)){

		motivoLiberacionCPU = MOT_FINALIZO;

	}else if(instruccionesEjecutadas == quantum){

		motivoLiberacionCPU = MOT_QUANTUM;

	}

	if(!DTBBloqueado()){
		enviarMotivoyDatos(miDTB,motivoLiberacionCPU,instruccionesEjecutadas,NULL);
	}

	limpiarVariables(miSentencia);
}


///GESTION DE CONEXIONES///

void gestionarConexionSAFA(){
	int ejecucion;
	while(1){
		if(myRecibirDatosFijos(socketSAFA,&ejecucion,sizeof(int))!=1){
			if(ejecucion == EJECUCION_NORMAL){

				recibirQyPlanificacion();

				DTB *miDTB;
				miDTB = recibirDTB(socketSAFA);

				myPuts("El DTB que se recibio es:\n");
				imprimirDTB(miDTB);

				if(miDTB->Flag_GDTInicializado == 0){
					operacionDummy(miDTB);
				}else{
					ejecutarInstruccion(miDTB);
				}

				//list_destroy(miDTB->tablaArchivosAbiertos);
				free(miDTB);
			}

			if(ejecucion == PREGUNTAR_DESCONEXION_CPU){
				ejecucion = 4; 								//No me interesa el valor que le mando
				myEnviarDatosFijos(socketSAFA,&ejecucion,sizeof(int));
			}

		}else{
			myPuts(RED "Se desconecto el proceso S-AFA" COLOR_RESET "\n");

			exit(1);
		}
	}

}


void gestionarConexionDAM(int socketDAM){

}


void gestionarConexionFM9(){

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
		myPuts(RED "No se encuentra disponible el DAM para conectarse" COLOR_RESET "\n");
		exit(1);
	}

	socketGFM9 = socketFM9;
	gestionarConexionFM9();
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

	socketGDAM = socketDAM;
	gestionarConexionDAM(socketDAM);
	return 0;
}


void* connectionSAFA(){
	u_int32_t result;

	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*)getConfigR("S-AFA_IP",0,configCPU));
	PUERTO_ESCUCHA=(int)getConfigR("S-AFA_PUERTO",1,configCPU);

	result=myEnlazarCliente((int*)&socketSAFA,IP_ESCUCHA,PUERTO_ESCUCHA);

	if(result==1){
		myPuts("No se encuentra disponible el S-AFA para conectarse.\n");
		exit(1);
	}

	gestionarConexionSAFA();

	return 0;
}

	///MAIN///

int main() {
	tiempoI = time(NULL);

	system("clear");
	pthread_t hiloConnectionDAM;
	pthread_t hiloConnectionSAFA;
	pthread_t hiloConnectionFM9;

	sem_init(&semDAM,0,0);

	configCPU=config_create(PATHCONFIGCPU);

	mostrarConfig();

    pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);
    pthread_create(&hiloConnectionSAFA,NULL,(void*)&connectionSAFA,NULL);
    pthread_create(&hiloConnectionFM9,NULL,(void*)&connectionFM9,NULL);

    while(1)
    {

    }
	config_destroy(configCPU);
	return EXIT_SUCCESS;
}
