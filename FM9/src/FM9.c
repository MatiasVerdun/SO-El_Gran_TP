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
char* memoriaFM9;

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

void miLiberarSplit(char ** vecStrings,int cantLineas){
	for(int i = 0; i < cantLineas; i++){
		free(vecStrings[i]);
	}
	free(vecStrings);
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

int agregarFilaTPI(int idDTB){

	for(int i = 0; i < (tamMemoria/tamPagina); i++){
			if(!framesOcupados[i]){
				framesOcupados[i] = 1;
				tablaDePaginasInvertidas[i]->ID_GDT =idDTB;
				tablaDePaginasInvertidas[i]->pagina = paginaGlobal;
				tablaDePaginasInvertidas[i]->cantPaginas = 0;

				paginaGlobal++;
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

		for(int j = 0; j < tamMemoria/tamLinea; j++){
			printf("%d",lineasOcupadas[j]);
		}
		printf("\n");

		int cantConjuntos;
		if(myRecibirDatosFijos(GsocketDAM,&cantConjuntos,sizeof(int))==1)
			myPuts(RED "Error al recibir la cantidad de Conjuntos" COLOR_RESET "\n");

		char* conjuntos = recibirDatosTS(GsocketDAM,maxTransfer);

		char **vecStrings = bytesToLineas(conjuntos);

		for(int i = 0;  i < cantLineas; i++){
			memcpy(memoriaFM9+(miSegmento->base + i)*tamLinea,vecStrings[i],strlen(vecStrings[i]));
		}
		free(conjuntos);
		//liberarSplit(vecStrings);
		int fileID = miSegmento->fileID;
		myEnviarDatosFijos(GsocketDAM,&fileID,sizeof(int));

	} else {
		int hayEspacio = 1;
		myEnviarDatosFijos(GsocketDAM,&hayEspacio,sizeof(int));
		myPuts(RED "No hay espacio" COLOR_RESET "\n");
	}

}

void abrirArchivoTPI(int cantLineas){
	int idDTB,cantFrames,i,j;
	int offsetUltimoFrame = 0;
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
			offsetUltimoFrame = ((cantLineas*tamLinea)%tamPagina)/tamLinea;
		}

		int cantConjuntos;
		if(myRecibirDatosFijos(GsocketDAM,&cantConjuntos,sizeof(int))==1)
			myPuts(RED "Error al recibir la cantidad de Conjuntos" COLOR_RESET "\n");

		char* conjuntos = recibirDatosTS(GsocketDAM,maxTransfer);

		char **vecStrings = bytesToLineas(conjuntos);

		free(conjuntos);

		int frame = agregarFilaTPI(idDTB);
		filaTPI* fila = tablaDePaginasInvertidas[frame];
		int dirLogica = fila->pagina * tamPagina;
		fila->cantPaginas = cantFrames;

		if(cantFrames == 1 && offsetUltimoFrame==0){
			for(i = 0;  i < (tamPagina/tamLinea); i++){
				memcpy(memoriaFM9+(((frame * tamPagina)/tamLinea)+i)*tamLinea,vecStrings[i],strlen(vecStrings[i]));
			}
		}else if(cantFrames == 1 ){
			for(i = 0;  i < offsetUltimoFrame; i++){
				memcpy(memoriaFM9+(((frame * tamPagina)/tamLinea)+i)*tamLinea,vecStrings[i],strlen(vecStrings[i]));
			}
		}	else if(offsetUltimoFrame == 0){

			for(i = 0;  i < tamPagina/tamLinea; i++){
				memcpy(memoriaFM9+(((frame * tamPagina)/tamLinea)+i)*tamLinea,vecStrings[i],strlen(vecStrings[i]));
			}

			for(j = 1;  j < cantFrames; j++){
				frame =agregarFilaTPI(idDTB);
				for(i = 0;  i < (tamPagina/tamLinea); i++){
					memcpy(memoriaFM9+(((frame * tamPagina)/tamLinea)+i)*tamLinea,vecStrings[j*tamPagina/tamLinea+i],strlen(vecStrings[j*tamPagina/tamLinea+i]));
				}
			}
		}else{
			for(i = 0;  i < tamPagina/tamLinea; i++){
				memcpy(memoriaFM9+(((frame * tamPagina)/tamLinea)+i)*tamLinea,vecStrings[i],strlen(vecStrings[i]));
			}

			for(j = 1;  j < cantFrames-1; j++){
				frame =agregarFilaTPI(idDTB);
				for(int i = 0;  i < (tamPagina/tamLinea); i++){
					memcpy(memoriaFM9+(((frame * tamPagina)/tamLinea)+i)*tamLinea,vecStrings[j*tamPagina/tamLinea+i],strlen(vecStrings[j*tamPagina/tamLinea+i]));
				}
			}

			frame= agregarFilaTPI(idDTB);
			for( int k  = 0;  k < offsetUltimoFrame; k++){
				memcpy(memoriaFM9+(((frame * tamPagina)/tamLinea)+k)*tamLinea,vecStrings[(j-1)*tamPagina/tamLinea+i+k],strlen(vecStrings[(j-1)*tamPagina/tamLinea+i+k]));
			}
		}

		myEnviarDatosFijos(GsocketDAM,&dirLogica,sizeof(int));

		miLiberarSplit(vecStrings,cantLineas);

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

	strcat(datos,"\n");

	if(strlen(datos) < tamLinea){
		base  = buscarBasePorfileID(fileID);

		limite = buscarLimitePorfileID(fileID);

		if(limite >= linea){

			int tamMemset = strlen(memoriaFM9+(base+(linea-1))*tamLinea);

			memset(memoriaFM9+(base+(linea-1))*tamLinea,'\0',tamMemset+1); //Capaz que se necesita cambiar

			memcpy(memoriaFM9+(base+(linea-1))*tamLinea,datos,strlen(datos));

			myPuts(GREEN "Operacion Asignar correcta." COLOR_RESET "\n");
			return 0;
		}else{
			myPuts(RED " Fallo de segmento/memoria." COLOR_RESET "\n");
			return 1;
		}
	}else{
		myPuts(RED "Espacio insuficiente en FM9." COLOR_RESET "\n");
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

	memset(memoriaFM9+(frame*tamPagina+offset),'\0',tamLinea+1);

	memcpy(memoriaFM9+(frame*tamPagina+offset),datos,strlen(datos));

	myPuts(GREEN "Operacion Asignar correcta." COLOR_RESET "\n");

	return 0;
}

void asignarLineaSPA(){

}

void asignarLinea(int socketCPU){
	int fileID,linea,tamanioDatos,respuesta;
	char* datos;

	if(myRecibirDatosFijos(socketCPU,&fileID,sizeof(int))==1)
		myPuts(RED"Error al recibir el fileID"COLOR_RESET"\n");
	if(myRecibirDatosFijos(socketCPU,&linea,sizeof(int))==1)
		myPuts(RED"Error al recibir la linea "COLOR_RESET"\n");
	if(myRecibirDatosFijos(socketCPU,&tamanioDatos,sizeof(int))==1)
		myPuts(RED"Error al recibir el tamaño"COLOR_RESET"\n");

	datos = malloc(tamanioDatos+2);
	memset(datos,'\0',tamanioDatos+2);

	if(myRecibirDatosFijos(socketCPU,datos,tamanioDatos)==1)
		myPuts(RED"Error al recibir los datos"COLOR_RESET"\n");

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

	free(datos);
	myEnviarDatosFijos(socketCPU,&respuesta,sizeof(int));
}

	/// FLUSH ///

void flushSEG(int fileID){
	int base;
	int limite;
	int tamanio=0;
	char* paqueteEnvio;

	base = buscarBasePorfileID(fileID);
	limite = buscarLimitePorfileID(fileID);

	for(int i = base;i<limite+base;i++){
		tamanio += strlen(memoriaFM9+i*tamLinea);
	}

	paqueteEnvio = malloc(tamanio+1);
	memset(paqueteEnvio,'\0',tamanio+1);
	for(int i = base;i<limite+base;i++){
		strcat(paqueteEnvio,memoriaFM9+i*tamLinea);
	}

	enviarDatosTS(GsocketDAM,paqueteEnvio,maxTransfer);

}

void flushTPI(int dirLogica){
	char* paqueteEnvio;
	int frame;
	int tamanio=0;
	int pagina = dirLogica / tamPagina;
	int cantFrames;
	int primerFrame;

	primerFrame = buscarFramePorPagina(pagina);
	filaTPI* fila = tablaDePaginasInvertidas[primerFrame];
	cantFrames = fila->cantPaginas;

	for(int i = 0;i<tamPagina/tamLinea;i++){
		tamanio += strlen(memoriaFM9+(primerFrame*tamPagina+i*tamLinea));
	}

	for(int i = 1;i<cantFrames;i++){
		frame = buscarFramePorPagina(pagina+i);
		for(int j = 0;j<tamPagina/tamLinea;j++){
			tamanio += strlen(memoriaFM9+(frame*tamPagina+j*tamLinea));
		}
	}

	paqueteEnvio = malloc(tamanio+1);
	memset(paqueteEnvio,'\0',tamanio+1);

	for(int i = pagina ; i < (pagina + cantFrames) ;i++){
		frame = buscarFramePorPagina(i);
		for(int j = 0;j<tamPagina/tamLinea;j++){
			strcat(paqueteEnvio,memoriaFM9+(frame*tamPagina+j*tamLinea));
		}
	}

	enviarDatosTS(GsocketDAM,paqueteEnvio,maxTransfer);
	free(paqueteEnvio);
}

void flushSPA(){

}

void flush(){
	int fileID;

	if(myRecibirDatosFijos(GsocketDAM,&fileID,sizeof(int))==1)
		myPuts(RED "Error al recibir el file ID" COLOR_RESET "\n");

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
		memset(memoriaFM9+i*tamLinea,'\0',tamLinea);
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

	int cantFrames = pagina + tablaDePaginasInvertidas[primerFrame]->cantPaginas;

	for(int i = pagina ; i < cantFrames ;i++){
		frame = buscarFramePorPagina(i);
		framesOcupados[frame]=0;			//Desocupa el frame
		memset(memoriaFM9+(frame*tamPagina),'\0',tamPagina);
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

void cerrarVariosArchivosSEG(int idDTB){

	for(int i = 0; i < list_size(tablaDeSegmentos);i++){
		SegmentoDeTabla* segmento = list_get(tablaDeSegmentos,i);

		if(segmento->ID_GDT == idDTB){

			cerrarArchivo(segmento->fileID);

		}
	}
}

void cerrarVariosArchivosTPI(int idDTB){
	filaTPI* fila;
	for(int i = 0; i < tamMemoria/tamPagina;i++){
		fila = tablaDePaginasInvertidas[i];

		if(fila->ID_GDT == idDTB && fila->cantPaginas > 0){

			cerrarArchivo(fila->pagina*tamPagina);

		}
	}
}

void cerrarVariosArchivos(){
	int idDTB;
	if(myRecibirDatosFijos(GsocketDAM,&idDTB,sizeof(int))==1)
		myPuts(RED "Error al recibir el idDTB" COLOR_RESET "\n");

	switch(modoEjecucion){
	case SEG:
		cerrarVariosArchivosSEG(idDTB);
		break;
	case TPI:
		cerrarVariosArchivosTPI(idDTB);
		break;
	case SPA:
		//cerrarArchivoSPA();
		break;
	}
	myPuts(GREEN "Se cerraron correctamente todos los archivos del DTB NRO: %d" COLOR_RESET "\n",idDTB);

}

	/// ENVIAR LINEA  ///

void enviarLineaSEG(int socketCPU, int fileID, int linea){
	char* strLinea= malloc(tamLinea+1);
	memset(strLinea,'\0',tamLinea+1);
	int base = buscarBasePorfileID(fileID);

	memcpy(strLinea,memoriaFM9+(base+linea)*tamLinea,strlen(memoriaFM9+(base+linea)*tamLinea));				//TODO Marian no esta convencido

	int tamanio = strlen(strLinea);

	myEnviarDatosFijos(socketCPU,&tamanio,sizeof(int));

	myEnviarDatosFijos(socketCPU,strLinea,tamanio);

	free(strLinea);
}

void enviarLineaTPI(int socketCPU, int dirLogica, int linea){
	char* strLinea = malloc(tamLinea+1);
	memset(strLinea,'\0',tamLinea+1);

	int pagina = (dirLogica + linea) / tamPagina;

	int offset = (dirLogica + linea) % tamPagina;

	int frame = buscarFramePorPagina(pagina);

	memcpy(strLinea,memoriaFM9+((frame * tamPagina)/tamLinea + offset)*tamLinea,strlen(memoriaFM9+((frame * tamPagina)/tamLinea + offset)*tamLinea));	//TODO Marian no esta convencido

	int tamanio = strlen(strLinea);

	myEnviarDatosFijos(socketCPU,&tamanio,sizeof(int));

	myEnviarDatosFijos(socketCPU,strLinea,tamanio);

	free(strLinea);
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
		//enviarLineaSPA(socketCPU,fileID,linea);
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

	tamMemoria=(int) getConfigR("TMM",1,configFM9);
	memoriaFM9=malloc(tamMemoria+1);
	memset(memoriaFM9,'\0',tamMemoria+1);

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
	free(myText);
	myText = string_from_format("Tam. maximo de memoria: %s", (char*)getConfigR("TMM",0,configFM9) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("Tam. linea: %s\0", (char*)getConfigR("TML",0,configFM9) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
	free(myText);
	myText = string_from_format("Tam. de pagina: %s\0", (char*)getConfigR("TMP",0,configFM9) );
	displayBoxBody(50,myText);
	displayBoxClose(50);
    free(myText);
}

	///GESTION DE CONEXIONES///

void ocuparLineas(int base, int num){
	for(int i=0; i<num; i++){
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

				case (OPERACION_CLOSE): //Si DAM envia esta operacion es porque termino un DTB

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

					myPuts(GREEN "OPERACION CLOSE finalizada correctamente" COLOR_RESET "\n");

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

	tamLinea=(int) getConfigR("TML",1,configFM9);;

	inicializarMemoria();


	if(modoEjecucion == SEG)
	{
		myPuts(BLUE "--------- El Modo de Ejecucion es SEGMENTACION PURA --------- " COLOR_RESET "\n");
		if(inicializarLineasOcupadas()==-1)
			myPuts(RED "El tamaño de memoria no es multiplo del tamaño de linea" COLOR_RESET "\n");
		tablaDeSegmentos = list_create();
	} else if(modoEjecucion == TPI)
	{
		tamPagina = (int) getConfigR("TMP",1,configFM9);
		myPuts(BLUE "--------- El Modo de Ejecucion es TABLA DE PAGINAS INVERTIDAS --------- " COLOR_RESET "\n");
		if(inicializarFramesOcupados()==-1)
			myPuts(RED "El tamaño de memoria no es multiplo del tamaño de pagina" COLOR_RESET "\n");
		tablaDePaginasInvertidas = malloc(4*tamMemoria/tamPagina);
		for(int i = 0; i< tamMemoria/tamPagina;i++){
			tablaDePaginasInvertidas[i] = malloc(sizeof(filaTPI));
			tablaDePaginasInvertidas[i]->pagina = -1;
			tablaDePaginasInvertidas[i]->ID_GDT = -1;
			tablaDePaginasInvertidas[i]->cantPaginas = -1;
		}
	} else if(modoEjecucion == SPA)
	{
		myPuts(BLUE "--------- El Modo de Ejecucion es SEGMENTACION PAGINADA --------- " COLOR_RESET "\n");
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
