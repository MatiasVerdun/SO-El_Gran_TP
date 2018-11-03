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
int instruccionesEjecutadas;
int abortarEjecucionModoDIABLOskereee = 0;
int motivoLiberacionCPU = -1; // 0-> Quantum , 1->Finalizo , 2-> Bloqueo , 3-> Error , 4-> Abortar

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

	resultRecv = myRecibirDatosFijos(socketSAFA,&retardoPlanificacion,sizeof(int));
	if(resultRecv !=0){
		printf("Error al recibir el retardo de la planificacion");
	}

	if(strcmp(tipoPlanificacion,"FIFO")==0){
			printf("Recibi que la planificacion es FIFO \n ");
	}else{
			printf("Recibi que la planificacion es %s y el Quantum es %d \n",tipoPlanificacion,quantum);
	}
}

void recibirRemanente(){
	int resultRecv;

	resultRecv = myRecibirDatosFijos(socketSAFA,&remanente,sizeof(int));

	if(resultRecv !=0){
			printf("Error al recibir el Remanente");
	}

}

void operacionDummy(DTB *miDTB){
	int largoRuta;
	char *strLenRuta;

	largoRuta = strlen(miDTB->Escriptorio);

	strLenRuta = string_from_format("%03d",largoRuta);

	myPuts("Enviando al Diego la ruta del Escriptorio.\n");

	int estado = 2;

	int inst = 0;

	enviarDTByMotivo(miDTB,estado,inst); // Avisar a S-AFA del block y sin instrucciones ejecutadas

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

bool validarArchivoEnLaLista(t_list *listaArchivos,char* nombreArchivo){
	for (int indice = 0;indice < list_size(listaArchivos);indice++){
		datosArchivo *miArchivo;
		miArchivo = list_get(listaArchivos,indice);
		if(strcmp(miArchivo->nombreArchivo,nombreArchivo) == 0){
				return true;
		}
	}
	return false;
}

void gestionDeSentencia(DTB *miDTB,sentencia *miSentencia){
	bool existeArchivoAbierto;

	switch(miSentencia->operacion){

		case OPERACION_ABRIR:
			existeArchivoAbierto = validarArchivoEnLaLista(miDTB->tablaArchivosAbiertos,miSentencia->param1);
			if(!existeArchivoAbierto){
				operacionDummy(miDTB);
			}else{
				//SiguienteSentencia
			}
		break;
		case OPERACION_CONCENTRAR:
		break;
		case OPERACION_ASIGNAR:
		break;
		case OPERACION_WAIT:
		break;
		case OPERACION_SIGNAL:
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
}

void ejecutarInstruccion(DTB* miDTB){
	int instruccionesEjecutadas = 0;

	while(hayQuantum(instruccionesEjecutadas) && !terminoElDTB() && !DTBBloqueado() && (abortarEjecucionModoDIABLOskereee == 0)){

		estoyEjecutando = 1;

		sentencia *dudoso; //TODO

		gestionDeSentencia(miDTB,dudoso);

		miDTB->PC++;
		instruccionesEjecutadas++;

	}
	if(abortarEjecucionModoDIABLOskereee == 1){

		abortarEjecucionModoDIABLOskereee = 0;
		motivoLiberacionCPU = 4;

	} else if(terminoElDTB()){

		motivoLiberacionCPU = 1;
	} else if(DTBBloqueado()){

		motivoLiberacionCPU = 2;
	} else if(instruccionesEjecutadas == quantum){

		motivoLiberacionCPU =  0;
	}

	estoyEjecutando = 0;
	instruccionesEjecutadas = 0;
}

void enviarDTByMotivo(DTB* miDTB, int motivo, int instrucciones){
	char* strDTB;

	strDTB = DTBStruct2String (miDTB);

	myEnviarDatosFijos(socketSAFA,strDTB,strlen(strDTB));

	myEnviarDatosFijos(socketSAFA,&motivo,sizeof(int));

	myEnviarDatosFijos(socketSAFA,&instrucciones,sizeof(int));

}

///GESTION DE CONEXIONES///

int recibirModoDeEjecucion(){
	int resultRecv;
	int modo;

	resultRecv = myRecibirDatosFijos(socketSAFA,&modo,sizeof(int));
	if(resultRecv !=0){
		printf("Error al recibir el Modo de Ejecucion");
	}
	return modo;
}

void gestionarConexionSAFA(){

	while(1){
		int miModo = recibirModoDeEjecucion();
		DTB *miDTB;

		switch(miModo){
			case MODO_DTB:

				if(strcmp(tipoPlanificacion,"VRR")==0){
					recibirRemanente();
				}

				miDTB = recibirDTB(socketSAFA);

				myPuts("El DTB que se recibio es:\n");
				imprimirDTB(miDTB);

				if(miDTB->Flag_GDTInicializado == 0){
					operacionDummy(miDTB);
				}else{
					ejecutarInstruccion(miDTB);
					enviarDTByMotivo(miDTB,motivoLiberacionCPU,instruccionesEjecutadas);
				}
			break;

			case MODO_QyP:
				recibirQyPlanificacion();
			break;

			case MODO_FINALIZAR:
				abortarEjecucionModoDIABLOskereee = 1;
			break;
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
