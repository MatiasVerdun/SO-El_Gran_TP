/*
 * filesystemFIFA.c
 *
 *  Created on: 7 sep. 2018
 *      Author: utnso
 */
#include "filesystemFIFA.h"
#include <archivos/archivos.h>
#include <console/myConsole.h>
#include <commons/string.h>

void escribirDirectorioIndice(char* datos,int indice){//Escribe los datos del directorio en la posicion del indice indicado
   int fd,desplazamiento;
   struct stat mystat;
   size_t FILESIZE=0,textSize=0;
   char *data;  /* mmapped array of chars */
   fd = open(PATHD, O_RDWR | O_CREAT, S_IRWXU);
   if (fd == -1) {
	   perror("Error opening file for writing");
		exit(EXIT_FAILURE);
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
	   exit(EXIT_FAILURE);
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
			exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }

    if (fstat(fd, &fileInfo) == -1){
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }

    if (fileInfo.st_size == 0){
        fprintf(stderr, "Error: File is empty, nothing to do\n");
        exit(EXIT_FAILURE);
    }

    data = mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if (data == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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

