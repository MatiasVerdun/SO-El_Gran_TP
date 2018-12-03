/*
 * filesystemFIFA.c
 *
 *  Created on: 7 sep. 2018
 *      Author: utnso
 */
#include "filesystemFIFA.h"


void escribirMetadataArchivo(char* metadata,char* pathArchivoFS){
	char* puntoMontaje = string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathABSarchivo = string_from_format("%s/%s", puntoMontaje,pathArchivoFS);
	escribirArchivo(pathABSarchivo,metadata);
	free(puntoMontaje);
	free(pathABSarchivo);
}


void guardarBitmap(){
	FILE *fBitmap;
	char* puntoMontaje= string_from_format((char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathBitmap= string_from_format("%sMetadata/Bitmap.bin",puntoMontaje);
	int cantBytes=cantBloques/8;
	if(cantBloques%8!=0)
		cantBytes++;
	fBitmap = fopen(pathBitmap, "wb");
	fwrite(bitmap->bitarray,cantBytes,1,fBitmap);
	fclose(fBitmap);
	free(puntoMontaje);
	free(pathBitmap);
}


int getCantBloquesLibres(){
	int cantidad=0;
	for(int i=0;i<(bitmap->size);i++){
		if(bitarray_test_bit(bitmap,i)==0)
			cantidad++;
	}
	return cantidad;
}


void cargarBitmap(){
	char* puntoMontaje= string_from_format((char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathBitmap= string_from_format("%sMetadata/Bitmap.bin",puntoMontaje);
	size_t sizeArchivo=tamArchivo(pathBitmap);
	char *bitmapDatos=malloc(sizeArchivo);
	FILE *fBitmap;
	int cantBytes=cantBloques/8;
	if(cantBloques%8!=0)
		cantBytes++;
	fBitmap = fopen(pathBitmap, "r+b");

	fread(bitmapDatos, 1, cantBytes, fBitmap);
	fclose(fBitmap);
	bitmap = bitarray_create_with_mode(bitmapDatos, cantBloques, MSB_FIRST);
	free(puntoMontaje);
	free(pathBitmap);
	/*for(int i=0;i<(bitmap->size);i++){
		printf("%d",bitarray_test_bit(bitmap,i));
	}
	printf("\n");*/
}


void setBloqueOcupado(int index){
	bitarray_set_bit(bitmap,index);
	guardarBitmap();
}


void setBloqueLibre(int index){
	bitarray_clean_bit(bitmap,index);
	guardarBitmap();
}


int getNBloqueLibre(){
	for(int i=0;i<(bitmap->size);i++){
		if(bitarray_test_bit(bitmap,i)==0)
			return i;
	}
	return -1;
}


void mostrarBitmap(){
	for(int i=0;i<(bitmap->size);i++){
		if(i%64==0 && i!=0){
			printf("\n");
		}
		printf("%d",bitarray_test_bit(bitmap,i));
	}
	printf("\n");
}


void pruebaBitmap(int tipo){
	if(tipo==1){
		for(int i=0;i<(bitmap->size);i++){
			bitarray_set_bit(bitmap,i);
		}
	}else{
		for(int i=0;i<(bitmap->size);i++){
			bitarray_clean_bit(bitmap,i);
		}
	}
	guardarBitmap();
}


void cargarFS(){
	struct stat st = {0};
	t_config *configFS;
	char *metadata ;
	char* puntoMontaje= string_from_format((char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	if (stat(puntoMontaje, &st) == -1) {//TODO Crear nuevo punto de montaje
		printf("La carpeta %s no existe\n",puntoMontaje);
	}else{
		dirActual=string_from_format("%sArchivos/",puntoMontaje);
		metadata=string_from_format("%sMetadata/Metadata.bin", puntoMontaje);
		configFS=config_create(metadata);
		tamBloque=(int)getConfigR("TAMANIO_BLOQUES",1,configFS);
		cantBloques=(int)getConfigR("CANTIDAD_BLOQUES",1,configFS);
		printf("Tamanio bloques: %d\n", tamBloque);
		printf("Cantidad de bloques: %d\n", cantBloques);
		cargarBitmap();

	}
	free(puntoMontaje);
	free(metadata);
	config_destroy(configFS);
}


char* leerBloqueDesdeHasta(char* nroBloque,int offset,int size){
	char* contenidoBloque=malloc(size+1);
	char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	memset(contenidoBloque,'\0',size+1);
	char* pathBloque=string_from_format("%sBloques/%s.bin", puntoMontaje,nroBloque);
	leerArchivoDesdeHasta(pathBloque,contenidoBloque,offset,size);
	//printf("Contenido bloque %s:\n%s\n",nroBloque,contenidoBloque);
	free(puntoMontaje);
	free(pathBloque);
	return contenidoBloque;
}


char* leerBloque(char* nroBloque){
	char* contenidoBloque=malloc(tamBloque+1);
	char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	memset(contenidoBloque,'\0',tamBloque+1);
	char* pathBloque=string_from_format("%sBloques/%s.bin", puntoMontaje,nroBloque);
	leerArchivoDesdeHasta(pathBloque,contenidoBloque,0,tamBloque);
	//printf("Contenido bloque %s:\n%s\n",nroBloque,contenidoBloque);
	free(puntoMontaje);
	free(pathBloque);
	return contenidoBloque;
}


void escribirBloque(char* nroBloque,char* datos){
	char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathBloque=string_from_format("%sBloques/%s.bin", puntoMontaje,nroBloque);
	escribirArchivo(pathBloque,datos);
	setBloqueOcupado(atoi(nroBloque));
	free(puntoMontaje);
	free(pathBloque);
}


void escribirBloqueDesde(char* nroBloque,int inicio,char* datos){
	char* puntoMontaje= string_from_format((char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathBloque=string_from_format("%sBloques/%s.bin", puntoMontaje,nroBloque);
	//printf("%s\n",datos);
	escribirArchivoDesde(pathBloque,datos,inicio);
	free(puntoMontaje);
	free(pathBloque);
}


void limpiarBloque(char* nroBloque){
	char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathBloque=string_from_format("%sBloques/%s.bin", puntoMontaje,nroBloque);
	limpiarArchivo(pathBloque);
	setBloqueLibre(atoi(nroBloque));
	free(puntoMontaje);
	free(pathBloque);
}


char** obtenerBloquesArchivoFS(char* pathArchivoFS){
	t_config *configFS;
	char** bloques;
	char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char *pathABSarchivo=string_from_format("%s/%s", puntoMontaje,pathArchivoFS);
	configFS=config_create(pathABSarchivo);
	bloques=config_get_array_value(configFS, "BLOQUES");
	config_destroy(configFS);
	free(puntoMontaje);
	free(pathABSarchivo);
	return bloques;
}


char* obtenerArchivoFS(char* pathFSArchivo){ //pathFSArchivo-> Path del archivo en el FileSystem Fifa, pathABSArchivo-> Path absoluto del archivo en filesystem Unix
	struct stat st = {0};
	t_config *configFS;

	char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	//u_int32_t tamArchivo;
	u_int32_t i=0;
	char** bloques;

	if (stat(puntoMontaje, &st) == -1) {
	    //mkdir("/some/directory", 0700);
		printf("La carpeta %s no existe\n",puntoMontaje);
		return "ERROR";
	}else{
		if(existeArchivoFS(pathFSArchivo)==0){
			char *pathABSarchivo=string_from_format("%s%s", puntoMontaje,pathFSArchivo);

			configFS=config_create(pathABSarchivo);
			//tamArchivo=(int)getConfigR("TAMANIO",1,configFS);
			bloques=config_get_array_value(configFS, "BLOQUES");
			char *archivo=string_new();

			//printf("Tamanio : %d\n", tamArchivo);
			//printf("Cantidad de bloques: %d\n", cantBloquesArchivo);
			while(bloques[i]!=NULL){
				//printf("Bloque %d: %s\n",i,bloques[i]);
				char *contenidoBloque=(char*)leerBloque(bloques[i]);
				//printf("Contenido del bloque:\n%s\n",contenidoBloque);
				string_append(&archivo,contenidoBloque);
				free(contenidoBloque);
				i++;
			}
			free(puntoMontaje);
			free(pathABSarchivo);
			liberarSplit(bloques);
			config_destroy(configFS);
			return archivo;
		}else{
			free(puntoMontaje);
			return "ERROR";
		}
	}
}


char* obtenerPathCarpetaArchivoFS(char* pathArchivoFS){
	int i=0;
	char* carpetaArchivoFS=string_new();;
	char** splitRuta= string_split(pathArchivoFS,"/");

	while(NULL!=splitRuta[i]){
		if(splitRuta[i+1]!=NULL){
			string_append(&carpetaArchivoFS,splitRuta[i]);
			string_append(&carpetaArchivoFS,"/");
		}
		i++;
	}
	liberarSplit(splitRuta);
	return carpetaArchivoFS;
}


void crearCarpetas(char* pathArchivoFS){
	char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char** splitPathFS= string_split(pathArchivoFS,"/");
	struct stat st = {0};
	int i=0;
	char *dirACrear=string_from_format("%s%s",puntoMontaje,splitPathFS[0]);
	i++;
	while(splitPathFS[i]!=NULL){
		//printf("%s\n",dirACrear);
		if (stat(dirACrear, &st) == -1)
			    mkdir(dirACrear, 0700);
		string_append_with_format(&dirACrear,"/%s",splitPathFS[i]);
		i++;
	}
	free(puntoMontaje);
	free(dirACrear);
	liberarSplit(splitPathFS);
}


char* obtenerDirAnterior(char* path){
	char **splitPath=string_split(path,"/");
	char *dirAnterior=string_new();
	int i=0;
	while(splitPath[i+1]!=NULL){
		string_append(&dirAnterior,"/");
		string_append(&dirAnterior,splitPath[i]);
		i++;
	}
	string_append(&dirAnterior,"/");
	liberarSplit(splitPath);
	return dirAnterior;
}


int esRutaFS(char* posibleRuta){
	int i=0;
	while(posibleRuta[i]!=NULL){
		i++;
		if(posibleRuta[i]=='/')
			return 0;
	}
	return 1;
}


int validarPathArchivoFS(char* pathArchivoFS){
	char* carpetaArchivoFS = obtenerPathCarpetaArchivoFS(pathArchivoFS);
	int respuesta=0;
	respuesta = existeCarpetaFS(carpetaArchivoFS);

	free(carpetaArchivoFS);
	return respuesta;
}


int existeCarpetaFS(char* pathCarpetaFS){
	char* puntoMontaje = string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathABSCarpeta = string_from_format("%s%s", puntoMontaje,pathCarpetaFS);;
	struct stat st = {0};
	if (stat(pathABSCarpeta, &st) == -1){ //Si existe devuelve 0
		free(puntoMontaje);
		free(pathABSCarpeta);
		return 1;
	}else{
		free(puntoMontaje);
		free(pathABSCarpeta);
		return 0;
	}
}


int existeArchivoFS(char* pathArchivoFS){
	char* puntoMontaje = string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
	char* pathABSarchivo = string_from_format("%s/%s", puntoMontaje,pathArchivoFS);;
	FILE *f=fopen(pathABSarchivo,"r");
	if(f==NULL){
		free(puntoMontaje);
		free(pathABSarchivo);

		return 1;
	}else{
		free(puntoMontaje);
		free(pathABSarchivo);
		fclose(f);
		return 0;
	}
}


int esArchivoFS(char* pathArchivoFS){
	if(strncmp(pathArchivoFS,"Archivos",strlen("Archivos"))==0)
		return 0;
	else
		return 1;
}


int obtenerTamArchivoFS(char* pathFSArchivo){
	t_config *configFS;
	char *pathABSarchivo ;
	u_int32_t tamArchivo;
	char* puntoMontaje= string_from_format((char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));

	pathABSarchivo=string_from_format("%s/%s", puntoMontaje,pathFSArchivo);
	configFS=config_create(pathABSarchivo);
	tamArchivo=(int)getConfigR("TAMANIO",1,configFS);

	free(puntoMontaje);
	free(pathABSarchivo);
	config_destroy(configFS);
	return tamArchivo;
}


void escribirDirectorioIndice(char* datos,int indice){//Escribe los datos del directorio en la posicion del indice indicado
   int fd,desplazamiento;
   struct stat mystat;
   size_t FILESIZE=0,textSize=0;
   char *data;  /* mmapped array of chars */
   fd = open(PATHD, O_RDWR | O_CREAT, S_IRWXU);
   if (fd == -1) {
	   perror("Error opening file for writing");
		exit(0);
   }
   if(stat(PATHD,&mystat)<0){
	   perror("fstat");
	   close(fd);
	   exit(1);
   }

   FILESIZE=mystat.st_size;
   textSize=strlen(datos);

   data = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);

   if (data == MAP_FAILED) {
	   close(fd);
	   perror("Error mmapping the file");
	   exit(0);
   }
   desplazamiento=indice*tamDirectorio;

   memcpy(data+desplazamiento,datos,strlen(datos));

   msync(data,FILESIZE,MS_SYNC);
   if (munmap(data, textSize) == -1) {
	   perror("Error un-mmapping the file");
	   close(fd);
	   exit(1);
   }
   close(fd);
}


void actualizarArchivoDirectorio(struct tablaDirectory *t_directorios){// ESCRIBE TODOS LOS DATOS DEL STRUCT EN EL ARCHIVO
	//memset(buffer,0,sizeof(t_directorios));
	int i;
	char numberStr[12];

	//char buffer[2048];
	for (i=0;i<100;i++){
		char* buffer=string_new();
		sprintf(numberStr, "%d", t_directorios[i].index);
		string_append(&buffer,numberStr);
		string_append(&buffer,"\n");
		string_append(&buffer,t_directorios[i].nombre);
		string_append(&buffer,"\n");
		sprintf(numberStr, "%d", t_directorios[i].padre);
		string_append(&buffer,numberStr);
		string_append(&buffer,"\n");
		/*strcpy(buffer,(char*)string_itoa(t_directorios[i].index));
		strcat(buffer,"\n");
		strcat(buffer,t_directorios[i].nombre);
		strcat(buffer,"\n");
		strcat(buffer,(char*)string_itoa(t_directorios[i].padre));
		strcat(buffer,"\n");*/
		escribirDirectorioIndice(buffer,i);
		free(buffer);
		//memset(buffer,0,sizeof(t_directorios));
	}

}


int obtenerPadreDir(char* nombreDir){
	int i,resultado;
	i=resultado=0;
	tableDirectory t_directorio;
	while(resultado!=-1){
		resultado=leerArchivoDirectorio(&t_directorio,i);
		if(strcmp(t_directorio.nombre,nombreDir)==0){
			return t_directorio.padre;
		}
		i++;
	}
	return -1;
}


int obtenerIndiceDir(char* nombreDir){
	int i,resultado;
	i=resultado=0;
	tableDirectory t_directorio;
	while(resultado!=-1){
		resultado=leerArchivoDirectorio(&t_directorio,i);
		if(strcmp(t_directorio.nombre,nombreDir)==0){
			return t_directorio.index;
		}
		i++;
	}
	return -1;
}


int crearDirectorio(struct tablaDirectory *t_directorios,char* pathDir){
	int i=0,j=0;
	char strItoa[12];
	char *rutaDirMD=(char*) string_new();
	char **splitaux=(char**) string_split(pathDir,":");
	char **nombresDirectorios = (char**)string_split(splitaux[1],"/");
	struct stat st = {0};

	if(strcmp("fifa",splitaux[0])!=0){
		printf("Error: El path del directorio debe ser de la forma fifa: ... \n");
		exit(1);
	}else{
		while(NULL!=nombresDirectorios[i]){ //Agarro el ultimo nombre de directorio
			i++;
		}
		i--;
		for(j=i-1;j>=0;j--){ //Valido si los directorios padres existen
			if(obtenerIndiceDir(nombresDirectorios[j])==-1){
				printf("Error: Directorio padre <%s> no existe \n",nombresDirectorios[j]);
				return -1;
			}
		}
		j=0;
		if((obtenerPadreDir(nombresDirectorios[0])!=0) && (obtenerPadreDir(nombresDirectorios[0]))!=-1){
			printf("Error: Ruta invalida \n");
			return -1;
		}
		for(j=0;j>i;j++){
			if(obtenerIndiceDir(nombresDirectorios[j])==obtenerPadreDir(nombresDirectorios[j+1])){
				printf("Error: Ruta invalida \n");
				return -1;
			}
		}

		if(obtenerIndiceDir(nombresDirectorios[i])!=-1){ //Si el directorio existe no lo crea e informa por consola
			printf("Error: El directorio ya existe\n");
			return -1;
		}else{
			j=0;
			while(t_directorios[j+1].padre!=-100){
				j++;
			}
			j++;
			strcpy(t_directorios[j].nombre,nombresDirectorios[i]);
			if(i==0){
				t_directorios[j].padre=0; //Si el directorio tiene nivel 1 su padre es el root(De indice 0)
			}else{
				t_directorios[j].padre=obtenerIndiceDir(nombresDirectorios[i-1]);//Obtengo el indice del directorio padre y lo asigno
			}
		}
	}
	string_append(&rutaDirMD,"/home/utnso/git/tp-2017-2c-Rafaga-de-amor-/FileSystem/Metadata/archivos/");

	sprintf(strItoa, "%d", t_directorios[j].index);
	string_append(&rutaDirMD,strItoa);
	if (stat(rutaDirMD, &st) == -1) {
	    mkdir(rutaDirMD, 0700);
	}
	printf("Se creo el directorio <%s> correctamente \n",nombresDirectorios[i]);
	liberarSplit(splitaux);
	liberarSplit(nombresDirectorios);
	free(rutaDirMD);
	return 0;
}


void crearDirectorioRoot(struct tablaDirectory *t_directorios){
	tableDirectory *directorio=malloc(sizeof(tableDirectory));
	directorio->index=0;
	strcpy(directorio->nombre,"root");
	directorio->padre=-500;
	t_directorios[0]=*directorio;
	free(directorio);
}


void inicializarTdir(struct tablaDirectory *t_directorios){
	int i;
	for(i=0;i<100;i++){
		t_directorios[i].index=i;
		strcpy(t_directorios[i].nombre,"null");
		t_directorios[i].padre=-100;
	}
}


void inicializarDir(struct tablaDirectory *t_directorios){
	inicializarTdir(t_directorios);
	crearDirectorioRoot(t_directorios);
	actualizarArchivoDirectorio(t_directorios);
}


void crearArchivoDirectorio(){
	   int fd;
	   struct stat mystat;
	   fd = creat(PATHD, S_IRWXU);
	   if (fd == -1) {
		   perror("Error opening file for writing");
			exit(0);
	   }
	   truncate(PATHD,tamMaxDirectorios);
	   if(stat(PATHD,&mystat)<0){
		   perror("fstat");
		   close(fd);
		   exit(1);
	   }
}


void crearMetadata(){
	tableDirectory t_directorios[100];

	if((existeArchivo(PATHD))==1){
		crearArchivoDirectorio();
		inicializarDir(t_directorios);
		//printf("Se creo el directorios.dat \n");
	}else{
		//printf("directorios.dat ya creado\n");
	}

}


void listarPadresDir(struct tablaDirectory *t_directorios,int i){
	if(i<100){
		if(obtenerPadreDir(t_directorios[i].nombre)!=0){
			listarPadresDir(t_directorios,t_directorios[obtenerPadreDir(t_directorios[i].nombre)].index);
			printf(CYAN "/%s" COLOR_RESET,t_directorios[obtenerPadreDir(t_directorios[i].nombre)].nombre);
		}
	}
}


void listarDirectorios(struct tablaDirectory *t_directorios,int i,int nivel){
	int j=1;
	if(i==0)
		printf(CYAN"root(/)" COLOR_RESET "\n");
	if(i<100){
		while(j<=99){
			if(t_directorios[j].padre==i){
				for(int aux=0;aux<nivel;aux++){
					printf(CYAN "|"COLOR_RESET);
					printf(CYAN "-----" COLOR_RESET);

				}
				printf(CYAN "|" COLOR_RESET);
				printf(CYAN "---" COLOR_RESET);
				if(t_directorios[j].padre!=0){
					listarPadresDir(t_directorios,j);
				}
				printf(CYAN "/%s" COLOR_RESET "\n",t_directorios[j].nombre);
				nivel++;
				listarDirectorios(t_directorios,j,nivel);
				nivel--;
			}
			j++;
		}
	}
}


int validarPathDir(char* pathDir){
	char** nombresDirectorios,**splitaux;
	int i=0,j=0;
	splitaux= (char**)string_split(pathDir,":");
	if(strcmp("fifa",splitaux[0])!=0){
		printf("Error: El path del directorio debe ser de la forma fifa: ... \n");
		return -1;
	}else{
		nombresDirectorios = (char**)string_split(splitaux[1],"/");
		while(NULL!=nombresDirectorios[i]){ //Agarro el ultimo nombre de directorio
			i++;
		}
		i--;
		if(obtenerIndiceDir(nombresDirectorios[i])==-1){
			printf("Error: el directorio <%s> no existe\n",nombresDirectorios[i]);
			return -1;
		}
		for(j=i-1;j>=0;j--){ //Valido si los directorios padres existen
			if(obtenerIndiceDir(nombresDirectorios[j])==-1){
				printf("Error: Directorio padre <%s> no existe \n",nombresDirectorios[j]);
				return -1;
			}
		}
		j=0;
		if((obtenerPadreDir(nombresDirectorios[0])!=0) && (obtenerPadreDir(nombresDirectorios[0]))!=-1){
			printf("Error: Ruta invalida \n");
			return -1;
		}
		for(j=0;j>i;j++){
			if(obtenerIndiceDir(nombresDirectorios[j])==obtenerPadreDir(nombresDirectorios[j+1])){
				printf("Error: Ruta invalida \n");
				return -1;
			}
		}

	}
	liberarSplit(splitaux);
	liberarSplit(nombresDirectorios);
	return 0;
}


int borrarDirectorio(struct tablaDirectory *t_directorios,char* pathDir){
	int i,j,k,resultado,resultadoAux;
	char **nombresDirectorios,**splitaux;
	i=resultado=j=k=resultadoAux=0;
	tableDirectory t_directorio,t_directorioAux;
	splitaux= (char**)string_split(pathDir,":");
	nombresDirectorios =(char**) string_split(splitaux[1],"/");

	while(NULL!=nombresDirectorios[k]){ //Agarro el ultimo nombre de directorio
		k++;
	}
	k--;
	if(obtenerIndiceDir(nombresDirectorios[k])==-1){
		printf("Error: el directorio <%s> no existe\n",nombresDirectorios[k]);
		return -1;
	}
	while(resultado!=-1){
		resultado=leerArchivoDirectorio(&t_directorio,i);
		if(strcmp(t_directorio.nombre,nombresDirectorios[k])==0){
			while(resultadoAux!=-1){//Verifico si el directorio esta vacio
				resultadoAux=leerArchivoDirectorio(&t_directorioAux,j);
				if(t_directorioAux.padre==t_directorio.index){
					printf("Error: el directorio no esta vacio\n");
					return -1;
				}
				j++;
			}
			strcpy(t_directorios[t_directorio.index].nombre,"null");
			t_directorios[t_directorio.index].padre=-100;
		}
		i++;
	}
	if(j==0){
		printf("Error: el directorio <%s> no existe\n",nombresDirectorios[k]);
		return -1;
	}
	else{
		printf("Se borro el directorio <%s> correctamente \n",nombresDirectorios[k]);
	}
	liberarSplit(splitaux);
	liberarSplit(nombresDirectorios);
	return 0;
}


int leerArchivoDirectorio(struct tablaDirectory *t_directorios,int numeroDirectorio){
    int i,fd,posicionDirectorioActual,posicionDirectorioSiguiente,j=0;
    struct stat fileInfo = {0};
    char* buffer=malloc(sizeof(tableDirectory));
    char* data;
    memset(buffer,'\0',sizeof(tableDirectory));
    fd=open(PATHD, O_RDWR, (mode_t)0600);
    if (fd == -1){
        perror("Error opening file for writing");
        exit(0);
    }

    if (fstat(fd, &fileInfo) == -1){
        perror("Error getting the file size");
        exit(0);
    }

    if (fileInfo.st_size == 0){
        fprintf(stderr, "Error: File is empty, nothing to do\n");
        exit(0);
    }

    data = mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if (data == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(0);
    }
    posicionDirectorioActual=tamDirectorio*numeroDirectorio;
    posicionDirectorioSiguiente= tamDirectorio*(numeroDirectorio+1);
	if(data[posicionDirectorioActual]=='\0'){
		free(buffer);
		return -1;
	}else{
		for (i = posicionDirectorioActual; i < posicionDirectorioSiguiente; i++)
		{
			if(data[i]!='\0'){
				//printf("%c",data[i]);
				buffer[j]=data[i];
				j++;
			}
		}
		j=0;
		//printf("%s\n",buffer);
	    char **split=(char**)string_split(buffer,"\n");
		t_directorios->index=atoi(split[0]);
		strcpy(t_directorios->nombre,split[1]);
		t_directorios->padre=atoi(split[2]);

		liberarSplit(split);
	}
    //buffer[i+1]='\0';
    // Don't forget to free the mmapped memory
    if (munmap(data, fileInfo.st_size) == -1)
    {
        close(fd);
        perror("Error un-mmapping the file");
        exit(0);
    }
    free(buffer);
    close(fd);
    return 0;
}


void cargarStructDirectorio(struct tablaDirectory *t_directorios){
	int i,resultado;
	i=resultado=0;
	while(resultado!=-1){
		resultado=leerArchivoDirectorio(&t_directorios[i],i);
		i++;
	}
}


int array_length(void* array){
	if(array)
		return (sizeof(array)/sizeof(array[0]))+1;
	else
		return -1;
}

char* convertirAPathFS(char* path){

}


