#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
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

t_list *listaSentencias;

//Configuracion de Planificacion
int quantum;
int remanente;
int retardoPlanificacion;
char tipoPlanificacion[5];
bool estoyEjecutando = false;
bool terminoInstrucciones = false;
bool estaBloqueado = false;
int instruccionesEjecutadas = 0; //TODO Capaz que no hace falta que sea global
int motivoLiberacionCPU;
int codigoError = 0;


///FUNCIONES DE CONFIG///

void mostrarConfig(){

    char* myText = string_from_format("DAM   -> IP: %s - Puerto: %s\0",(char*)getConfigR("DAM_IP",0,configCPU), (char*)getConfigR("DAM_PUERTO",0,configCPU) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("S-AFA -> IP: %s - Puerto: %s\0", (char*)getConfigR("S-AFA_IP",0,configCPU), (char*)getConfigR("S-AFA_PUERTO",0,configCPU) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Retardo: %s\0" , (char*)getConfigR("RETARDO",0,configCPU) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

//

void recibirQyPlanificacion(){

	int resultRecv;

	char bufferPlanificacion[5];
	resultRecv = myRecibirDatosFijos(socketSAFA,bufferPlanificacion,5);
	if(resultRecv !=0){
		printf("Error al recibir el tipo de planificacion");
	}

	strcpy(tipoPlanificacion,bufferPlanificacion);


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
	}
}

void recibirRespuestaWaitSignal(){
	int resultRecv;
	int resultado;

	resultRecv = myRecibirDatosFijos(socketSAFA,&resultado,sizeof(int));
	if(resultRecv !=0){
			printf("Error al recibir el Remanente");
	}

	if(resultado == 0){
		motivoLiberacionCPU = MOT_BLOQUEO;
	}
}

//

void operacionDummy(DTB *miDTB){
	int largoRuta;
	char *strLenRuta;

	largoRuta = strlen(miDTB->Escriptorio);

	strLenRuta = string_from_format("%03d",largoRuta);

	myPuts("Enviando al Diego la ruta del Escriptorio.\n");

	int motivo = MOT_BLOQUEO;

	int inst = 0;

	enviarMotivoyDatos(miDTB,motivo,inst,NULL); // Avisar a S-AFA del block y sin instrucciones ejecutadas

	myEnviarDatosFijos(socketGDAM,strLenRuta,3);

	myEnviarDatosFijos(socketGDAM,miDTB->Escriptorio,largoRuta);
}

bool hayQuantum(int inst){
	if(strcmp(tipoPlanificacion,"VRR")==0 && remanente > 0){
		return inst < remanente;
	}

	return inst < quantum || quantum < 0;
}

bool terminoElDTB(){
	return terminoInstrucciones;
}

bool DTBBloqueado(){
	return estaBloqueado;
}

bool hayError(){
	return codigoError != 0;
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


void gestionDeSentencia(DTB *miDTB,sentencia *miSentencia){
	int fileID;
	int parametro2;
	int operacion = miSentencia->operacion;
	int tamanio;

	usleep((int)getConfigR("RETARDO",1,configCPU)); //CONCENTRAR

	switch(operacion){

		case OPERACION_ABRIR: //AL DIEGO OP PATH
			fileID = buscarFileID(miDTB->tablaArchivosAbiertos,miSentencia->param1);

			if(fileID == -1){
				estaBloqueado = true; //DESALOJO DTB

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION

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

				myEnviarDatosFijos(socketGFM9,&fileID,sizeof(int));			//ENVIO ID

				//PARAMETROS

				tamanio = strlen(miSentencia->param1);
				myEnviarDatosFijos(socketGFM9,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO
				myEnviarDatosFijos(socketGFM9,miSentencia->param1,tamanio); //ENVIO EL PATH

				parametro2= miSentencia->param2;
				myEnviarDatosFijos(socketGFM9,&parametro2,sizeof(int)); //ENVIO LA LINEA

				tamanio = strlen(miSentencia->param3);
				myEnviarDatosFijos(socketGFM9,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO DE LOS DATOS
				myEnviarDatosFijos(socketGFM9,miSentencia->param3,tamanio); //ENVIO DATOS

			}else{
				codigoError = 20001;										//ERROR: El archivo no se encuentra abierto
			}

		break;

		case OPERACION_WAIT:
			enviarMotivoyDatos(NULL,ACC_WAIT,-1,miSentencia->param1);

			recibirRespuestaWaitSignal();
		break;

		case OPERACION_SIGNAL:
			enviarMotivoyDatos(NULL,ACC_SIGNAL,-1,miSentencia->param1);

			recibirRespuestaWaitSignal();
		break;

		case OPERACION_FLUSH: // AL DIEGO PARA FM9 OP ID PATH
			fileID = buscarFileID(miDTB->tablaArchivosAbiertos,miSentencia->param1);

			if(fileID != -1){
				estaBloqueado = true; //DESALOJO DTB

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION

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
			if(fileID == -1){
				estaBloqueado = true; //DESALOJO DTB

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION

				tamanio = strlen(miSentencia->param1);
				myEnviarDatosFijos(socketGDAM,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO
				myEnviarDatosFijos(socketGDAM,miSentencia->param1,tamanio); //ENVIO EL PATH

				parametro2= miSentencia->param2;
				myEnviarDatosFijos(socketGDAM,&parametro2,sizeof(int)); 	//ENVIO LA CANT DE LINEAS

			}else{
				codigoError = 50001;										//ERROR: El archivo ya existe
			}
		break;

		case OPERACION_BORRAR:// AL DIEGO PARA MDJ PATH
			if(fileID != -1){
				estaBloqueado= true; //DESALOJO DTB

				myEnviarDatosFijos(socketGDAM,&operacion,sizeof(int)); 		//ENVIO OPERACION

				tamanio = strlen(miSentencia->param1);
				myEnviarDatosFijos(socketGDAM,&tamanio,sizeof(int)); 		//ENVIO EL TAMAÑO
				myEnviarDatosFijos(socketGDAM,miSentencia->param1,tamanio); //ENVIO EL PATH

			}else{
				codigoError = 60001;										//ERROR: El archivo no existe
			}
		break;
	}

}

void ejecutarInstruccion(DTB* miDTB){

	while(hayQuantum(instruccionesEjecutadas) && !terminoElDTB() && !DTBBloqueado() && !hayError()){

		estoyEjecutando = true;

		sentencia *dudoso; //TODO

		gestionDeSentencia(miDTB,dudoso);

		miDTB->PC++;

		instruccionesEjecutadas++;

	}
	 if(terminoElDTB()){

		motivoLiberacionCPU = MOT_FINALIZO;

	}  else if(DTBBloqueado()){

		motivoLiberacionCPU = MOT_BLOQUEO;

	} else if(instruccionesEjecutadas == quantum){

		motivoLiberacionCPU =  MOT_QUANTUM;

	}else if (hayError()){
		motivoLiberacionCPU =  MOT_ERROR;

	}

	enviarMotivoyDatos(miDTB,motivoLiberacionCPU,instruccionesEjecutadas,NULL);

	estoyEjecutando = false;
	instruccionesEjecutadas = 0;
}

///PARSEAR SCRIPTS///


void parsear(char * linea){
	sentencia *laSentencia;

	listaSentencias = list_create();
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
		} else {
			fprintf(stderr, "La linea <%s> no es valida\n", linea);
			exit(EXIT_FAILURE);
		}

		list_add(listaSentencias,laSentencia);

	if (linea)
		free(linea);

	myPuts("El archivo parseado es el siguiente: \n");
	list_iterate(listaSentencias,(void*)imprimirSentencia);
}

///GESTION DE CONEXIONES///


void gestionarConexionSAFA(){
	while(1){
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

	}
}


void gestionarConexionDAM(int socketDAM){
	/*while(1){
		if(gestionarDesconexion((int)socketDAM,"DAM")!=0)
			break;
	}*/
}


void gestionarConexionFM9(int socketFM9){
	while(1){
		if(gestionarDesconexion((int)socketFM9,"FM9")!=0)
			break;
	}
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
		myPuts("No se encuentra disponible el DAM para conectarse.\n");
		exit(1);
	}

	socketGFM9 = socketFM9;
	gestionarConexionFM9(socketFM9);
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
	system("clear");
	pthread_t hiloConnectionDAM;
	pthread_t hiloConnectionSAFA;
	pthread_t hiloConnectionFM9;

	configCPU=config_create(PATHCONFIGCPU);

	mostrarConfig();

    pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);
    pthread_create(&hiloConnectionSAFA,NULL,(void*)&connectionSAFA,NULL);
    //pthread_create(&hiloConnectionFM9,NULL,(void*)&connectionFM9,NULL);

    while(1)
    {

    }

	return EXIT_SUCCESS;
}
