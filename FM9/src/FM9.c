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

char modoEjecucion[4];
t_config *configFM9;
void* memoriaFM9;
typedef struct SegmentoDeTabla {
	int fileID; //Se me ocurre que sea el mismo ID que el de la tabla de archivos abiertos del S-AFA y que se pase hasta aca
	int base; //en lineas
	int limite; //en lineas
} SegmentoDeTabla;

int* lineasOcupadas;

t_list tablaDeSegmentos; //TODO create_list() en algun lado

	/// ABRIR ARCHIVO ///

int espacioMaximoLibre(){

	int max = 0;
	int c = 0;

	for(int i = 0; i < (sizeof(lineasOcupadas)/sizeof(int)); i++){
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

	for(int i = 0; i < (sizeof(lineasOcupadas)/sizeof(int)); i++){
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

void abrirArchivo(){
	switch(modoEjecucion){
	case "SEG":
		abrirArchivoSEG();
		break;
	case "TPI":
		abrirArchivoTPI();
		break;
	case "SPA":
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

void asignarLinea(){
	switch(modoEjecucion){
	case "SEG":
		asignarLineaSEG();
		break;
	case "TPI":
		asignarLineaTPI();
		break;
	case "SPA":
		asignarLineaSPA();
		break;
	}
}

	/// FLUSH ///

void flushSEG(){

}

void flushTPI(){

}

void flushSPA(){

}

void flush(){
	switch(modoEjecucion){
	case "SEG":
		flushSEG();
		break;
	case "TPI":
		flushTPI();
		break;
	case "SPA":
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

void cerrarArchivo(){
	switch(modoEjecucion){
	case "SEG":
		cerrarArchivoSEG();
		break;
	case "TPI":
		cerrarArchivoTPI();
		break;
	case "SPA":
		cerrarArchivoSPA();
		break;
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

	///FUNCIONES DE CONFIG///

void setModoEjecucion(){
	modoEjecucion = (char *) getConfig("MODO_EJ","FM9.txt",1);
	modoEjecucion[3] = '\0';
}

int inicializarLineasOcupadas(){
	int tamMemoria=(int)getConfig("TMM","FM9.txt",1);
	int tamLinea=(int)getConfig("TML","FM9.txt",1);

	if((tamMemoria % tamLinea) != 0){
		printf("El tamaño de memoria no es multiplo del tamaño de linea");
		return -1;
	}

	lineasOcupadas = malloc(tamMemoria/tamLinea);

	for(int i = 0; i < (sizeof(lineasOcupadas)/sizeof(int)); i++){
		lineasOcupadas[i] = 0;
	}

}

void inicializarMemoria(){

	int tamMemoria=(int)getConfig("TMM","FM9.txt",1);
	memoriaFM9=malloc(tamMemoria);

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

void guardarScript(char* script){

}

void recibirScript(int socketDAM){
	u_int32_t respuesta=0;
	u_int32_t tamScript=0;
	myPuts(BLUE "Obteniendo script");
	loading(1);
	myRecibirDatosFijos(socketDAM,(u_int32_t*)&tamScript,sizeof(u_int32_t)); //Recibo el size
	char* buffer= malloc(ntohl(tamScript)+1);
	myRecibirDatosFijos(socketDAM,(char*)buffer,ntohl(tamScript));//TODO Hacer que reciba bien los datos (Recibir cuantos datos se van a mandar para luego recibirlos)
	/*memset(buffer+ntohl(tamScript),'\0',1);*/
	//myPuts("%s",buffer);
	//free(buffer);
	guardarDatos(buffer,ntohl(tamScript),0);
	leerDatos(ntohl(tamScript),0);
}

void gestionarConexionDAM(int *sock){
	int socketDAM = *(int*)sock;
	u_int32_t buffer=0,operacion=0;
	while(1){

		if(myRecibirDatosFijos(socketDAM,(u_int32_t*)&buffer,sizeof(u_int32_t))==0){
			operacion=ntohl(buffer);

			switch(operacion){
				case(1):
					recibirScript(socketDAM);
					break;
				/*case OPERACION_ABRIR:
					//  TODO rcv tamaño en lineas, chequear si hay espacio comparando el tamaño contra espacioMaximoLibre()
					// responder por si o por no
					// Si hay espacio, rcv datos, llamar a abrirArchivo(), break
				case OPERACION_ASIGNAR:
					//TODO idealmente se recibe la linea y posicion en la linea en la cual tengo que asignar
					//(no se si DAM sabe tanto) llamar a AsignarLinea(), break
				case OPERACION_FLUSH:
					//TODO se recibe la primera linea del archivo a flushear (y no se si el tamaño), flush()
					//hago un super send del archivo en cuestion, break
				case OPERACION_CLOSE:
					//TODO se recibe el archivo a cerrar (y no se si el tamaño), cerrarArchivo(), break
				*/

			}
		}else{
			myPuts(RED "Se desconecto el proceso DAM" COLOR_RESET "\n");
			break;
		}

	}
}

void gestionarConexionCPU(int *sock){
	int socketCPU = *(int*)sock;
	while(1){
		if(gestionarDesconexion((int)socketCPU,"CPU")!=0)
			break;
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
		myPuts("No fue posible conectarse con los procesos DAM");
		exit(1);
	}

	result = myAtenderClientesEnHilos((int*) &socketCPU, "FM9", "DAM",(void*) gestionarConexionDAM);
	if (result != 0) {
		myPuts("No fue posible atender requerimientos de DAM");
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

	//setModoEjecucion();

    pthread_create(&hiloConnectionDAM,NULL,(void*)&connectionDAM,NULL);
    //pthread_create(&hiloConnectionCPU,NULL,(void*)&connectionCPU,NULL);
	inicializarMemoria();
	pruebaGuardadoDTB();

    while(1)
    {

    }

    //config_destroy(configFM9);
	return EXIT_SUCCESS;
}




