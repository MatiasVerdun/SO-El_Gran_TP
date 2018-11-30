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
int tamPagina;

int modoEjecucion;
t_config *configFM9;
char** memoriaFM9;

typedef struct SegmentoDeTabla {
	int fileID; //Se me ocurre que sea el mismo ID que el de la tabla de archivos abiertos del S-AFA y que se pase hasta aca
	int base; //en lineas
	int limite; //en lineas
	int ID_GDT;
} SegmentoDeTabla;

typedef struct filaTPI {
	int pagina; //en lineas
	int ID_GDT;
	int cantPaginas; //Solo en la primera pagina
} filaTPI;

int GsocketDAM;
bool *lineasOcupadas;
bool *framesOcupados;
int GfileID = 0;
int maxTransfer;
int paginaGlobal=0;

t_list* tablaDeSegmentos;
filaTPI** tablaDePaginasInvertidas;

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

filaTPI* crearTPI(int idDTB){
	filaTPI* fila = malloc(sizeof(filaTPI));
	fila->pagina = paginaGlobal;
	fila->ID_GDT = idDTB;
	fila->cantPaginas = 0;

	paginaGlobal++;
	return fila;
}

/// BUSCAR///

int buscarFramePorPagina(int pagina){

	for(int i = 0 ; i < (tamMemoria/tamPagina); i++){
		if(tablaDePaginasInvertidas[i]->pagina == pagina){
			return i;
		}
	}
	return -1;
}

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

	/// ABRIR ARCHIVO ///

int espacioMaximoLibre(bool* estructura,int tamanio){

	int max = 0;
	int c = 0;

	for(int i = 0; i < (tamMemoria/tamanio); i++){
		if(!estructura[i]){
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

int  espacioLibre(bool* framesOcupados,int tamPagina){
	int c = 0;

	for(int i = 0; i < (tamMemoria/tamPagina); i++){
		if(!framesOcupados[i]){
				c++;
		}
	}
	return c;
}

int primeraLineaLibreDelEspacioMaximo(bool* estructura, int tamanio){

	int max = 0;
	int c = 0;
	int posMax = -1;
	int posActual = 0;

	for(int i = 0; i < (tamMemoria/tamanio); i++){
		if(!estructura[i]){
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

int agregarFilaTPI(filaTPI* fila){

	for(int i = 0; i < (tamMemoria/tamPagina); i++){
			if(!framesOcupados[i]){
				framesOcupados[i] = 1;
				tablaDePaginasInvertidas[i] = fila;
				return i;
			}
	}
	return -1;
}

void abrirArchivoSEG(int cantLineas){
	int idDTB;
	myPuts(BLUE "Obteniendo script");
	loading(1);

	int maxEspacioLibre = espacioMaximoLibre(lineasOcupadas, tamLinea);

	if(maxEspacioLibre >= cantLineas){
		int hayEspacio = 0;
		myEnviarDatosFijos(GsocketDAM,&hayEspacio,sizeof(int));

		if(myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int))==1)
				myPuts(RED "Error al recibir la cantidad de Conjuntos" COLOR_RESET "\n");

		SegmentoDeTabla *miSegmento = crearSegmento();
		miSegmento->base = primeraLineaLibreDelEspacioMaximo(lineasOcupadas,tamLinea);
		miSegmento->limite = cantLineas;
		miSegmento->ID_GDT = idDTB;

		list_add(tablaDeSegmentos,miSegmento);

		ocuparLineas(miSegmento->base,miSegmento->limite);

		int cantConjuntos;
		if(myRecibirDatosFijos(GsocketDAM,&cantConjuntos,sizeof(int))==1)
			myPuts(RED "Error al recibir la cantidad de Conjuntos" COLOR_RESET "\n");

		char* conjuntos = recibirDatosTS(GsocketDAM,ntohl(maxTransfer));

		char **vecStrings = bytesToLineas(conjuntos);

		for(int i = 0;  i < cantLineas; i++){
			memoriaFM9[miSegmento->base + i] = vecStrings[i];
		}

		int fileID = miSegmento->fileID;
		myEnviarDatosFijos(GsocketDAM,&fileID,sizeof(int));

	} else {
		int hayEspacio = 1;
		myEnviarDatosFijos(GsocketDAM,&hayEspacio,sizeof(int));
		myPuts(RED "No hay espacio" COLOR_RESET "\n");
	}

}

void abrirArchivoTPI(int cantLineas){
	int idDTB,cantFrames,offsetUltimoFrame;
	myPuts(BLUE "Obteniendo script");
	loading(1);

	int maxEspacioLibre = espacioLibre(framesOcupados,tamPagina);

	if(maxEspacioLibre >= cantLineas){
		int hayEspacio = 0;
		myEnviarDatosFijos(GsocketDAM,&hayEspacio,sizeof(int));

		if(myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int))==1)
			myPuts(RED "Error al recibir la cantidad de Conjuntos" COLOR_RESET "\n");

		cantFrames = (cantLineas*tamLinea)/tamPagina;

		if((cantLineas*tamLinea)%tamPagina != 0){
			cantFrames++;
			offsetUltimoFrame = (cantLineas*tamLinea)%tamPagina;
		}

		int cantConjuntos;
		if(myRecibirDatosFijos(GsocketDAM,&cantConjuntos,sizeof(int))==1)
			myPuts(RED "Error al recibir la cantidad de Conjuntos" COLOR_RESET "\n");

		char* conjuntos = recibirDatosTS(GsocketDAM,ntohl(maxTransfer));

		char **vecStrings = bytesToLineas(conjuntos);

		filaTPI* fila = crearTPI(idDTB);
		int frame = agregarFilaTPI(fila);
		int dirLogica = fila->pagina * tamPagina;
		fila->cantPaginas = cantFrames;

		if(cantFrames == 1 && offsetUltimoFrame==0){
			for(int i = 0;  i < (tamPagina/tamLinea); i++){
				memoriaFM9[(frame * tamPagina)+i] = vecStrings[i];
			}
		}else if(cantFrames == 1 ){
			for(int i = 0;  i < offsetUltimoFrame; i++){
				memoriaFM9[(frame * tamPagina)+i] = vecStrings[i];
			}
		}	else if(offsetUltimoFrame == 0){

			for(int j = 1;  j < cantFrames; j++){
				fila = crearTPI(idDTB);
				frame =agregarFilaTPI(fila);
				for(int i = 0;  i < (tamPagina/tamLinea); i++){
						memoriaFM9[(frame * tamPagina)+i] = vecStrings[i];
				}
			}
		}else{
			for(int j = 1;  j < cantFrames-1; j++){
				fila = crearTPI(idDTB);
				frame =agregarFilaTPI(fila);
				for(int i = 0;  i < (tamPagina/tamLinea); i++){
					memoriaFM9[(frame * tamPagina)+i] = vecStrings[i];
				}
			}
			fila = crearTPI(idDTB);
			frame =agregarFilaTPI(fila);
			for(int i = 0;  i < offsetUltimoFrame; i++){
				memoriaFM9[(frame * tamPagina)+i] = vecStrings[i];
			}
		}

		myEnviarDatosFijos(GsocketDAM,&dirLogica,sizeof(int));

		} else {
			int hayEspacio = 1;
			myEnviarDatosFijos(GsocketDAM,&hayEspacio,sizeof(int));
			myPuts(RED "No hay espacio" COLOR_RESET "\n");
		}
}

void abrirArchivoSPA(){

}

void abrirArchivo(int cantLineas){
	switch(modoEjecucion){
	case SEG:
		abrirArchivoSEG(cantLineas);
		break;
	case TPI:
		abrirArchivoTPI(cantLineas);
		break;
	case SPA:
		abrirArchivoSPA();
		break;
	}

}

	/// ASIGNAR LINEA ///

int  asignarLineaSEG(int fileID, int linea, char* datos){
	int base;
	int limite;

	if(strlen(datos) < tamLinea){
		base  = buscarBasePorfileID(fileID);

		limite = buscarLimitePorfileID(fileID);

		if(limite-base >= (linea-1)){

			memset(memoriaFM9[base+(linea-1)],'\0',tamLinea+1);

			memoriaFM9[base+(linea-1)] = datos;

			return 0;
		}else{
			return 1;
		}
	}else{
		return 2;
	}
}

int asignarLineaTPI(int dirLogica,int linea,char*datos){

	if(strlen(datos) > tamLinea){
		return 2;
	}

	int pagina = (dirLogica + linea-1) / tamPagina;

	int offset = (dirLogica + linea -1) % tamPagina;

	int frame = buscarFramePorPagina(pagina);

	memset(memoriaFM9[frame*tamPagina+offset],'\0',tamLinea+1);

	memoriaFM9[frame*tamPagina+offset] = datos;

	return 0;

}

void asignarLineaSPA(){

}

void asignarLinea(int socketCPU){
	int fileID,linea,tamanioDatos,respuesta;
	char* datos = NULL;

	myRecibirDatosFijos(socketCPU,&fileID,sizeof(int));
	myRecibirDatosFijos(socketCPU,&linea,sizeof(int));
	myRecibirDatosFijos(socketCPU,&tamanioDatos,sizeof(int));
	myRecibirDatosFijos(socketCPU,datos,tamanioDatos);

	switch(modoEjecucion){
	case SEG:
		respuesta = asignarLineaSEG(fileID, linea, datos);
	break;
	case TPI:
		respuesta = asignarLineaTPI(fileID,linea,datos);
		break;
	case SPA:
		asignarLineaSPA();
		break;
	}

	myPuts(GREEN "Operacion Asignar correcta." COLOR_RESET "\n");
	myEnviarDatosFijos(socketCPU,&respuesta,sizeof(int));
}

	/// FLUSH ///

void flushSEG(int fileID){
	int base;
	int limite;
	char* paqueteEnvio=NULL;

	base = buscarBasePorfileID(fileID);
	limite = buscarLimitePorfileID(fileID);

	for(int i = base;i<limite;i++){
		strcat(paqueteEnvio,memoriaFM9[i]);
	}

	enviarDatosTS(GsocketDAM,paqueteEnvio,maxTransfer);

}

void flushTPI(int dirLogica){
	char* paqueteEnvio=NULL;
	int frame;
	int pagina = dirLogica / tamPagina;
	int primerFrame= buscarFramePorPagina(pagina);

	for(int i = pagina ; i < (pagina + tablaDePaginasInvertidas[primerFrame]->cantPaginas) ;i++){
		frame = buscarFramePorPagina(i);
		for(int j = frame ; j < frame + tamPagina/tamLinea ;j++){
			strcat(paqueteEnvio,memoriaFM9[j]);
		}
	}

	enviarDatosTS(GsocketDAM,paqueteEnvio,maxTransfer);

}

void flushSPA(){

}

void flush(){
	int fileID;
	myRecibirDatosFijos(GsocketDAM,&fileID,sizeof(int));
	switch(modoEjecucion){
	case SEG:
		flushSEG(fileID);
		break;
	case TPI:
		flushTPI(fileID);
		break;
	case SPA:
		flushSPA();
		break;
	}
}

	/// CERRAR ARCHIVO ///

void cerrarArchivoSEG(int fileID){

	int base = buscarBasePorfileID(fileID);
	int limite = buscarLimitePorfileID(fileID);

	desocuparLineas(base,limite-base);

	for(int i = base; i < limite ;i++){
		memset(memoriaFM9[i],'\0',tamLinea);
	}

	for(int i = 0; i < list_size(tablaDeSegmentos);i++){
		SegmentoDeTabla* segmento = list_get(tablaDeSegmentos,i);
		if(segmento->fileID == fileID){
			list_remove_and_destroy_element(tablaDeSegmentos,i,(void*)free);
		}
	}

}

void cerrarArchivoTPI(int dirLogica){
	int frame;
	int pagina = dirLogica / tamPagina;
	int primerFrame= buscarFramePorPagina(pagina);

	for(int i = pagina ; i < (pagina + tablaDePaginasInvertidas[primerFrame]->cantPaginas) ;i++){
		frame = buscarFramePorPagina(i);
		framesOcupados[frame]=0;			//Desocupa el frame
		memset(memoriaFM9[frame*tamPagina],'\0',tamPagina);
		tablaDePaginasInvertidas[frame]->pagina = -1;
		tablaDePaginasInvertidas[frame]->ID_GDT = -1;
		tablaDePaginasInvertidas[frame]->cantPaginas= -1;
	}

}

void cerrarArchivoSPA(){

}

void cerrarArchivo(int fileID){

	switch(modoEjecucion){
	case SEG:
		cerrarArchivoSEG(fileID);
		break;
	case TPI:
		cerrarArchivoTPI(fileID);
		break;
	case SPA:
		cerrarArchivoSPA();
		break;
	}

}

void cerrarVariosArchivos(){
	int idDTB;
	if(myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int))==1)
		myPuts(RED "Error al recibir el idDTB" COLOR_RESET "\n");

	for(int i = 0; i < list_size(tablaDeSegmentos);i++){
		SegmentoDeTabla* segmento = list_get(tablaDeSegmentos,i);

		if(segmento->ID_GDT == idDTB){

			cerrarArchivo(segmento->fileID);

		}
	}
}

	/// ENVIAR LINEA  ///

void enviarLineaSEG(int socketCPU, int fileID, int linea){
	char* strLinea = malloc(tamLinea+1);
	memset(strLinea,'\0',tamLinea+1);

	int base = buscarBasePorfileID(fileID);

	strLinea = memoriaFM9[base+linea];					//TODO Marian no esta convencido



	int tamanio = strlen(strLinea);

	myEnviarDatosFijos(socketCPU,&tamanio,sizeof(int));

	myEnviarDatosFijos(socketCPU,strLinea,tamanio);

}

void enviarLineaTPI(int socketCPU, int dirLogica, int linea){
	char* strLinea = malloc(tamLinea+1);
	memset(strLinea,'\0',tamLinea+1);

	int pagina = (dirLogica + linea) / tamPagina;

	int offset = (dirLogica + linea) % tamPagina;

	int frame = buscarFramePorPagina(pagina);

	strLinea = memoriaFM9[frame*tamPagina + offset];	//TODO Marian no esta convencido

	int tamanio = strlen(strLinea);

	myEnviarDatosFijos(socketCPU,&tamanio,sizeof(int));

	myEnviarDatosFijos(socketCPU,strLinea,tamanio);

}

void enviarLinea(int socketCPU, int fileID, int linea){

	switch(modoEjecucion){
	case SEG:
		enviarLineaSEG(socketCPU,fileID,linea);
		break;
	case TPI:
		enviarLineaTPI(socketCPU,fileID,linea);
		break;
	case SPA:
		enviarLineaSPA(socketCPU,fileID,linea);
		break;
	}

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

int inicializarLineasOcupadas( ){

	if((tamMemoria % tamLinea) != 0){
		printf("El tamaño de memoria no es multiplo del tamaño de linea");
		return -1;
	}

	lineasOcupadas = malloc(tamMemoria/tamLinea);

	for(int i = 0; i < (tamMemoria/tamLinea); i++){
		lineasOcupadas[i] = 0;
	}


	return 0;
}

int inicializarFramesOcupados(){

	if((tamMemoria % tamPagina) != 0){
		printf("El tamaño de memoria no es multiplo del tamaño de pagina");
		return -1;
	}

	framesOcupados = malloc(tamMemoria/tamPagina);

	for(int i = 0; i < (tamMemoria/tamPagina); i++){
		framesOcupados[i] = 0;
	}


	return 0;
}

void inicializarMemoria(){

	tamMemoria=(int)getConfig("TMM","FM9.txt",1);
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

void desocuparLineas(int base, int num){
	for(int i=0; i<base+num; i++){
		lineasOcupadas[base+i] = 0;
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
	int operacion,fileID,linea,respuesta;

	while(1){
		if(myRecibirDatosFijos(socketCPU,&operacion,sizeof(int))!=1){
			switch(operacion){
				case (OPERACION_ASIGNAR):
					asignarLinea(socketCPU);
				break;

				case (OPERACION_CLOSE):
					if(myRecibirDatosFijos(socketCPU,&fileID,sizeof(int))==1)
						myPuts(RED"Error al recibir el fileID"COLOR_RESET"\n");

					cerrarArchivo(fileID);

					respuesta = 0;
					myEnviarDatosFijos(socketCPU,&respuesta,sizeof(int));

				break;

				case (OPERACION_LINEA):
					if(myRecibirDatosFijos(socketCPU,&fileID,sizeof(int))==1)
						myPuts(RED"Error al recibir el fileID"COLOR_RESET"\n");

					if(myRecibirDatosFijos(socketCPU,&linea,sizeof(int))==1)
						myPuts(RED"Error al recibir el numero de la linea"COLOR_RESET"\n");

					enviarLinea(socketCPU,fileID,linea);
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

	tamMemoria=(int)getConfig("TMM","FM9.txt",1);
	tamLinea=(int)getConfig("TML","FM9.txt",1);
	tamPagina = (int) getConfig("TMP","FM9.txt",1);

	inicializarMemoria();


	if(modoEjecucion == SEG)
	{
		if(inicializarLineasOcupadas()==-1)
			myPuts(RED "El tamaño de memoria no es multiplo del tamaño de linea" COLOR_RESET "\n");
		tablaDeSegmentos = list_create();
	} else if(modoEjecucion == TPI)
	{
		if(inicializarFramesOcupados()==-1)
				myPuts(RED "El tamaño de memoria no es multiplo del tamaño de pagina" COLOR_RESET "\n");
		tablaDePaginasInvertidas = malloc(tamMemoria/tamPagina);
	} else if(modoEjecucion == SPA)
	{
		tamPagina = (int) getConfigR("TMP",1,configFM9);
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
