#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <console/myConsole.h>
#include <conexiones/mySockets.h>
#include <dtbSerializacion/dtbSerializacion.h>
#include <sentencias/sentencias.h>

#define PATHCONFIGFM9 "/home/utnso/tp-2018-2c-smlc/Config/FM9.txt"

enum{ SEG,
	  TPI,
	  SPA};

int tamMemoria;
int tamLinea;

int modoEjecucion;
t_config *configFM9;
char** memoriaFM9;
typedef struct SegmentoDeTabla {
	int fileID; //Se me ocurre que sea el mismo ID que el de la tabla de archivos abiertos del S-AFA y que se pase hasta aca
	int base; //en lineas
	int limite; //en lineas
} SegmentoDeTabla;

int GsocketDAM;
bool *lineasOcupadas;
int GfileID = 0;
int maxTransfer;

t_list* tablaDeSegmentos; //TODO create_list() en algun lado

	/// ABRIR ARCHIVO ///

int espacioMaximoLibre(){

	int max = 0;
	int c = 0;

	for(int i = 0; i < (tamMemoria/tamLinea); i++){
		if(!lineasOcupadas[i]){
			c++;
			if(c > max){
				max=c;

			}

		} else {
			c=0;
		}
	}

	return max;
}

int primeraLineaLibreDelEspacioMaximo(){

	int max = 0;
	int c = 0;
	int posMax = -1;
	int posActual = 0;

	for(int i = 0; i < (tamMemoria/tamLinea); i++){
		if(!lineasOcupadas[i]){
			c++;
			if(c > max){
				max=c;
				posMax = posActual;

			}

		} else {
			c=0;
			posActual = i+1;
		}
	}

	return posMax;
}

void abrirArchivoSEG(int miFileID, int miTamArchivo, char* misDatos){

	int miBase = primeraLineaLibreDelEspacioMaximo();

	SegmentoDeTabla* miSegmento = malloc(sizeof(SegmentoDeTabla));

	miSegmento->fileID = miFileID;
	miSegmento->base = miBase;
	miSegmento->limite = miTamArchivo;

	list_add(tablaDeSegmentos, miSegmento);

	free(miSegmento);

	//TODO escritura de datos comenzando en memoriaFM9[miBase * tamLinea (del config)], en un for que vaya avanzando sobre
	//un vector de split string por cambio de lineas \n de misDatos
}

void abrirArchivoTPI(){

}

void abrirArchivoSPA(){

}

void abrirArchivo(int cantLineas){
	switch(modoEjecucion){
	case SEG:
		recibirScript(cantLineas);
		break;
	case TPI:
		abrirArchivoTPI();
		break;
	case SPA:
		abrirArchivoSPA();
		break;
	}

}

	/// ASIGNAR LINEA ///

void asignarLineaSEG(){

}

void asignarLineaTPI(){

}

void asignarLineaSPA(){

}

void asignarLinea(int socketCPU){
	int fileID,linea,tamanioDatos;
	char* datos = NULL;

	myRecibirDatosFijos(socketCPU,&fileID,sizeof(int));
	myRecibirDatosFijos(socketCPU,&linea,sizeof(int));
	myRecibirDatosFijos(socketCPU,&tamanioDatos,sizeof(int));
	myRecibirDatosFijos(socketCPU,datos,tamanioDatos);

	switch(modoEjecucion){
	case SEG:
		asignarLineaSEG();
		break;
	case TPI:
		asignarLineaTPI();
		break;
	case SPA:
		asignarLineaSPA();
		break;
	}
}

	/// FLUSH ///

int  buscarBasePorfileID(int fileId){
	for(int i = 0 ; i < list_size(tablaDeSegmentos); i++){

		SegmentoDeTabla * elemento;

		elemento = list_get(tablaDeSegmentos,i);

		if(elemento->fileID == fileId)
			return elemento->base;
	}

	return -1;
}

int  buscarLimitePorfileID(int fileId){
	for(int i = 0 ; i < list_size(tablaDeSegmentos); i++){

		SegmentoDeTabla * elemento;

		elemento = list_get(tablaDeSegmentos,i);

		if(elemento->fileID == fileId)
			return elemento->limite;
	}

	return -1;
}

void flushSEG(){
	int fileID;
	int base;
	int limite;
	char* paqueteEnvio;

	myRecibirDatosFijos(GsocketDAM,&fileID,sizeof(int));

	base = buscarBasePorfileID(fileID);
	limite = buscarLimitePorfileID(fileID);

}

void flushTPI(){

}

void flushSPA(){

}

void flush(){
	int fileID;
	myRecibirDatosFijos(GsocketDAM,&fileID,sizeof(int));
	switch(modoEjecucion){
	case SEG:
		flushSEG();
		break;
	case TPI:
		flushTPI();
		break;
	case SPA:
		flushSPA();
		break;
	}
}

	/// CERRAR ARCHIVO ///

void cerrarArchivoSEG(){

}

void cerrarArchivoTPI(){

}

void cerrarArchivoSPA(){

}

void cerrarArchivo(int fileID){
	switch(modoEjecucion){
	case SEG:
		cerrarArchivoSEG();
		break;
	case TPI:
		cerrarArchivoTPI();
		break;
	case SPA:
		cerrarArchivoSPA();
		break;
	}
}

void cerrarVariosArchivos(){
	int cantidadDeArchivos;
	if(myRecibirDatosFijos(GsocketDAM,&cantidadDeArchivos,sizeof(int))==1)
		myPuts(RED "Error al recibir la cantidad de archivos que se debe cerrar" COLOR_RESET "\n");

	for(int i = 0; i < cantidadDeArchivos;i++){
		int fileID;
		if(myRecibirDatosFijos(GsocketDAM,&fileID,sizeof(int))==1)
				myPuts(RED "Error al recibir el fileID en la posicion nro %d " COLOR_RESET "\n",i);

		cerrarArchivo(fileID);
	}
}

	/// TEMP ///

void guardarDatos(void* datos,int size,int base){

	memcpy(memoriaFM9+base,datos,size);

}

void leerDatos(int size,int base){
	char* datos=malloc(size);

	memcpy(datos,memoriaFM9+base,size);
	printf("Datos leidos de memoria:\n%s\n",datos);

	free(datos);
}

SegmentoDeTabla* crearSegmento(){
	SegmentoDeTabla *miSegmento;

	miSegmento = malloc(sizeof(SegmentoDeTabla));

	miSegmento->fileID = GfileID;
	miSegmento->base = -1;
	miSegmento->limite = -1;

	GfileID++;

	return miSegmento;
}

	///FUNCIONES DE CONFIG///

void setModoEjecucion(){
	char *miModoEjecucion;

	miModoEjecucion = (char *) getConfigR("MODO_EJ",0,configFM9);

	if(strcmp(miModoEjecucion, "SEG")==0)
	{
		modoEjecucion = SEG;
	} else if(strcmp(miModoEjecucion, "TPI")==0)
	{
		modoEjecucion = TPI;
	} else if(strcmp(miModoEjecucion, "SPA")==0)
	{
		modoEjecucion = SPA;
	}
}

int inicializarLineasOcupadas(){
	tamMemoria=(int)getConfig("TMM","FM9.txt",1);
	tamLinea=(int)getConfig("TML","FM9.txt",1);

	if((tamMemoria % tamLinea) != 0){
		printf("El tamaño de memoria no es multiplo del tamaño de linea");
		return -1;
	}

	lineasOcupadas = malloc(tamMemoria/tamLinea);

	for(int i = 0; i < (tamMemoria/tamLinea); i++){
		lineasOcupadas[i] = 0;
	}

	/*for(int i = 0; i < cantEntradas; i++){
		bArray[i] = false;
	}*/
}

void inicializarMemoria(){

	int tamMemoria=(int)getConfig("TMM","FM9.txt",1);
	memoriaFM9=malloc(tamMemoria);
	memset(memoriaFM9,'\0',tamMemoria);

}

DTB* crearDTB(char *rutaMiScript){
	DTB *miDTB; //Sin free por que sino cuando lo meto en la cola pierdo el elemento
	miDTB = malloc(sizeof(DTB));

	miDTB->ID_GDT = 1;
	strcpy(miDTB->Escriptorio,rutaMiScript);
	miDTB->PC = 0;
	miDTB->Flag_GDTInicializado = 1;
	miDTB->tablaArchivosAbiertos = list_create();

	return miDTB;
}

void mostrarConfig(){

    char* myText = string_from_format("DAM -> IP: %s - Puerto: %s ", (char*)getConfigR("IP_ESCUCHA",0,configFM9), (char*)getConfigR("DAM_PUERTO",0,configFM9) );

	displayBoxTitle(50,"CONFIGURACION");
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Tam. maximo de memoria: %s", (char*)getConfigR("TMM",0,configFM9) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Tam. linea: %s\0", (char*)getConfigR("TML",0,configFM9) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	myText = string_from_format("Tam. de pagina: %s\0", (char*)getConfigR("TMP",0,configFM9) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

	///GESTION DE CONEXIONES///

void ocuparLineas(int base, int num){
	for(int i=0; i<base+num; i++){
		lineasOcupadas[base+i] = 1;
	}
}

int contarBarraN (char *conj){
	int c = 0;
	for(int i=0; i < maxTransfer; i++){
		if(conj[i] == '\n')
			c++;
	}
	return c;
}

void guardarScript(char* script){

}

int tamSplit(char** split){
	int i=0;
	while(split[i]){
		i++;
	}
	return i;
}

void recibirScript(int cantLineas){
	u_int32_t respuesta=0;
	u_int32_t tamScript=0;
	//int cantLineas;
	myPuts(BLUE "Obteniendo script");
	loading(1);

	int maxEspacioLibre = espacioMaximoLibre();

	if(maxEspacioLibre >= cantLineas){
		int hayEspacio = 0;
		myEnviarDatosFijos(GsocketDAM,&hayEspacio,sizeof(int));

		SegmentoDeTabla *miSegmento = crearSegmento();
		miSegmento->base = primeraLineaLibreDelEspacioMaximo();
		miSegmento->limite = cantLineas;

		ocuparLineas(miSegmento->base,miSegmento->limite);

		int cantConjuntos;
		if(myRecibirDatosFijos(GsocketDAM,&cantConjuntos,sizeof(int))==1)
			myPuts(RED "Error al recibir la cantidad de Conjuntos" COLOR_RESET "\n");

		char* conjuntos = recibirDatosTS(GsocketDAM,ntohl(maxTransfer));

		char **vecStrings = bytesToLineas(conjuntos);

		for(int i = 0;  i < cantLineas; i++){
			memoriaFM9[miSegmento->base + i] = vecStrings[i];
		}

		/*for(int i = 0; i <= cantConjuntos; i++){

			char *conjARecibir = malloc(maxTransfer+1);

			if(myRecibirDatosFijos(GsocketDAM,conjARecibir,maxTransfer)==1)
				myPuts(RED "Error al recibir el conjunto nro %d" COLOR_RESET "\n",i);

		    if(contarBarraN(conjARecibir) == 0){
		        strcat(memoriaFM9[lineaMemoria], conjARecibir);

		    } else {

		        char **vecStrings = string_split(conjARecibir,"\n");

		       int cantElementos = tamSplit(vecStrings);

		        for(int j = 0; j < cantElementos; j++){

		            strcat(memoriaFM9[lineaMemoria + j], vecStrings[j]);
		        	printf("cantLineas %d",cantLineas);
		        }

		        lineaMemoria = lineaMemoria + strlen(vecStrings) - 1;

		    }
		}*/

		int fileID = miSegmento->fileID;
		myEnviarDatosFijos(GsocketDAM,&fileID,sizeof(int));

	} else {
		int hayEspacio = 1;
		myEnviarDatosFijos(GsocketDAM,&hayEspacio,sizeof(int));
	}
	/*//myRecibirDatosFijos(socketDAM,(u_int32_t*)&tamScript,sizeof(u_int32_t)); //Recibo el size
	//char* buffer= malloc(ntohl(tamScript)+1);
	//myRecibirDatosFijos(socketDAM,(char*)buffer,ntohl(tamScript));//TODO Hacer que reciba bien los datos (Recibir cuantos datos se van a mandar para luego recibirlos)
	//memset(buffer+ntohl(tamScript),'\0',1);
	//myPuts("%s",buffer);
	//free(buffer);
	//guardarDatos(buffer,ntohl(tamScript),0);
	//leerDatos(ntohl(tamScript),0);*/
}

void gestionarConexionDAM(int *sock){
	GsocketDAM = *(int*)sock;
	int operacion,cantLineas;

	if(myRecibirDatosFijos(GsocketDAM,&maxTransfer,sizeof(int))==1)
		myPuts(RED "Error al recibir el Max Transfer" COLOR_RESET "\n");

	while(1){
		if(myRecibirDatosFijos(GsocketDAM,&operacion,sizeof(int))!=1){
			switch(operacion){
				case(OPERACION_DUMMY):
						if(myRecibirDatosFijos(GsocketDAM,&cantLineas,sizeof(int))==1)
							myPuts(RED "Error al recibir la cantidad de lineas" COLOR_RESET "\n");
						abrirArchivo(cantLineas);
					break;
				case (OPERACION_ABRIR):
					if(myRecibirDatosFijos(GsocketDAM,&cantLineas,sizeof(int))==1)
							myPuts(RED "Error al recibir la cantidad de lineas" COLOR_RESET "\n");
					abrirArchivo(cantLineas);
				break;
				case (OPERACION_FLUSH):
					flush();
				break;
					//TODO se recibe la primera linea del archivo a flushear (y no se si el tamaño), flush()
					//hago un super send del archivo en cuestion
				case (OPERACION_CLOSE): //Si DAM envia esta operacion es porque termino un DTB
					//TODO se recibe el archivo a cerrar (y no se si el tamaño)
					cerrarVariosArchivos();
				break;
			}
		}else{
			myPuts(RED "Se desconecto el proceso DAM" COLOR_RESET "\n");
			break;
		}

	}
}

void gestionarConexionCPU(int *sock){
	int socketCPU = *(int*)sock;
	int operacion,fileID;

	while(1){
		if(myRecibirDatosFijos(socketCPU,&operacion,sizeof(int))!=1){
			switch(operacion){
				case (OPERACION_ASIGNAR):
					asignarLinea(socketCPU);
				break;

				case (OPERACION_CLOSE):
					myRecibirDatosFijos(socketCPU,&fileID,sizeof(int));
					cerrarArchivo(fileID);
				break;
			}
		}else{
			myPuts(RED "Se desconecto el proceso CPU" COLOR_RESET "\n");
			break;
		}
	}
}

	///FUNCIONES DE CONEXION///

void* connectionDAM()
{
	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t socketDAM; // Descriptor de socket a la escucha
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfigR("IP_ESCUCHA",0,configFM9));
	PUERTO_ESCUCHA=(int) getConfigR("DAM_PUERTO",1,configFM9);


	result = myEnlazarServidor((int*) &socketDAM, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos DAM");
		exit(1);
	}

	result = myAtenderClientesEnHilos((int*) &socketDAM, "FM9", "DAM",(void*) gestionarConexionDAM);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de DAM");
		exit(1);
	}

	return 0;
}

void* connectionCPU()
{
	struct sockaddr_in direccionServidor; // Direccion del servidor
	u_int32_t result;
	u_int32_t socketCPU; // Descriptor de socket a la escucha
	char IP_ESCUCHA[15];
	int PUERTO_ESCUCHA;

	strcpy(IP_ESCUCHA,(char*) getConfigR("IP_ESCUCHA",0,configFM9));
	PUERTO_ESCUCHA=(int) getConfigR("CPU_PUERTO",1,configFM9);


	result = myEnlazarServidor((int*) &socketCPU, &direccionServidor,IP_ESCUCHA,PUERTO_ESCUCHA); // Obtener socket a la escucha
	if (result != 0) {
		myPuts("No fue posible conectarse con los procesos CPU");
		exit(1);
	}

	result = myAtenderClientesEnHilos((int*) &socketCPU, "FM9", "CPU",(void*) gestionarConexionCPU);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de CPU");
		exit(1);
	}

	return 0;
}

	///MAIN///
void guardarDTB(DTB *miDTB){

	char* stringDTB=malloc(sizeof(DTB));

	memcpy(stringDTB,DTBStruct2String(miDTB),sizeof(DTB)-4);
	memcpy(memoriaFM9,stringDTB,sizeof(DTB));
	//printf("%s\n",stringDTB);

	free(stringDTB);
}

DTB* leerDTB(int posicion){
	DTB* miDTB=NULL;
	char* stringDTB=malloc(sizeof(DTB));

	memcpy(stringDTB,memoriaFM9+posicion,sizeof(DTB));
	miDTB=DTBString2Struct(stringDTB);

	free(stringDTB);
	return miDTB;
}

void pruebaGuardadoDTB(){
	DTB *miDTB;
	DTB *DTBaGuardar=crearDTB(PATHCONFIGFM9);

    guardarDTB(DTBaGuardar);
	miDTB=leerDTB(0);
    printf("Ruta escriptorio: %s\n",miDTB->Escriptorio);
    printf("Estado GDT: %d\n",miDTB->Flag_GDTInicializado);
    printf("ID GDT: %d\n",miDTB->ID_GDT);
	printf("Program Counter: %d\n",miDTB->PC);

	free(DTBaGuardar);
	free(miDTB);
}

int main() {
	system("clear");
	pthread_t hiloConnectionDAM;
	pthread_t hiloConnectionCPU;

	configFM9=config_create(PATHCONFIGFM9);

	mostrarConfig();
	setModoEjecucion();
	inicializarMemoria();
	if(inicializarLineasOcupadas()==-1)
			myPuts(RED "El tamaño de memoria no es multiplo del tamaño de linea" COLOR_RESET "\n");

	if(modoEjecucion == SEG)
	{
		tablaDeSegmentos = list_create();
	} else if(modoEjecucion == TPI)
	{
	} else if(modoEjecucion == SPA)
	{
	}

    pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);
    pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);
	//pruebaGuardadoDTB();

    while(1)
    {

    }

    config_destroy(configFM9);
	return EXIT_SUCCESS;
}




