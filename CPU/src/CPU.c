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

#define PATHCONFIGCPU "/home/utnso/tp-2018-2c-smlc/Config/CPU.txt"
t_config *configCPU;

u_int32_t socketGDAM;
u_int32_t socketSAFA;

//Configuracion de Planificacion
int quantum;
int remanente;
int retardoPlanificacion;
char tipoPlanificacion[5];
int  estoyEjecutando = 0 ; // 0 no 1 si
int instruccionesEjecutadas = 0;
int motivoLiberacionCPU;

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

void enviarDTByMotivo(DTB* miDTB, int motivo, int instrucciones){
	char* strDTB;

	strDTB = DTBStruct2String (miDTB);

	myEnviarDatosFijos(socketSAFA,strDTB,strlen(strDTB));

	myEnviarDatosFijos(socketSAFA,&motivo,sizeof(int));

	myEnviarDatosFijos(socketSAFA,&instrucciones,sizeof(int));

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

	enviarDTByMotivo(miDTB,motivo,inst); // Avisar a S-AFA del block y sin instrucciones ejecutadas

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
	return motivoLiberacionCPU != 1;
}

bool DTBBloqueado(){
	return motivoLiberacionCPU != 2;
}

bool validarArchivoEnLaLista(t_list *listaArchivos,char* pathArchivo){
	for (int indice = 0;indice < list_size(listaArchivos);indice++){
		datosArchivo *miArchivo;
		miArchivo = list_get(listaArchivos,indice);
		if(strcmp(miArchivo->pathArchivo,pathArchivo) == 0){
				return true;
		}
	}
	return false;
}

void enviarAccionWaitSignal(char *recurso, int accion){
	myEnviarDatosFijos(socketSAFA,&accion,sizeof(int));

	if(accion != ACC_NADA){
		int tamanio = strlen(recurso);
		myEnviarDatosFijos(socketSAFA,&tamanio,sizeof(int));

		myEnviarDatosFijos(socketSAFA,recurso,tamanio);
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

void gestionDeSentencia(DTB *miDTB,sentencia *miSentencia){
	bool existeArchivoAbierto;
	int accion = ACC_NADA;

	switch(miSentencia->operacion){

		case OPERACION_ABRIR:
			existeArchivoAbierto = validarArchivoEnLaLista(miDTB->tablaArchivosAbiertos,miSentencia->param1);
			if(!existeArchivoAbierto){
				operacionDummy(miDTB);
			}
		break;

		case OPERACION_CONCENTRAR:
			usleep((int)getConfigR("RETARDO",1,configCPU));
		break;

		case OPERACION_ASIGNAR:
		break;

		case OPERACION_WAIT:
			accion = ACC_WAIT;
		break;

		case OPERACION_SIGNAL:
			accion = ACC_SIGNAL;
		break;

		case OPERACION_FLUSH:
		break;
		case OPERACION_CLOSE:
		break;
		case OPERACION_CREAR:
		break;
		case OPERACION_BORRAR:
		break;
	}

	enviarAccionWaitSignal(miSentencia->param1,accion);
	if(accion != ACC_NADA){
		recibirRespuestaWaitSignal();
	}

}

void ejecutarInstruccion(DTB* miDTB){

	while(hayQuantum(instruccionesEjecutadas) && !terminoElDTB() && !DTBBloqueado()){

		estoyEjecutando = 1;

		sentencia *dudoso; //TODO

		gestionDeSentencia(miDTB,dudoso);

		miDTB->PC++;

		instruccionesEjecutadas++;

	}
	 if(terminoElDTB()){

		motivoLiberacionCPU = MOT_FINALIZO;
	} else if(DTBBloqueado()){

		motivoLiberacionCPU = MOT_BLOQUEO;
	} else if(instruccionesEjecutadas == quantum){

		motivoLiberacionCPU =  MOT_QUANTUM;
	}

	enviarDTByMotivo(miDTB,motivoLiberacionCPU,instruccionesEjecutadas);

	estoyEjecutando = 0;
	instruccionesEjecutadas = 0;
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
