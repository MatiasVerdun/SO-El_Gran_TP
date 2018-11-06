/*
 * archivos.c
 *
 *  Created on: 10/10/2017
 *      Author: utnso
 */
#include "archivos.h"
typedef struct paqueteDatos{
	u_int32_t tamContenido;
	char *contenido;
	char nombre[255];
}paqueteDatos;

int verificarCarpeta(char* path){
	struct stat st = {0};
	if (stat(path, &st) == -1)
		return 0;
	else
		return 1;
}

void leerArchivo(char* FILEPATH,char* buffer){
    //const char *filepath = "/tmp/mmapped.bin";
    int i,j=0,fd;
    struct stat fileInfo = {0};
    char* data;
    fd=open(FILEPATH, O_RDWR, (mode_t)0600);

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

    if (data == MAP_FAILED){
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < fileInfo.st_size; i++){
		if(NULL!=data[i]){
			//printf("%c",data[i]);
			buffer[j]=data[i];
			j++;
		}
    }
    if (munmap(data, fileInfo.st_size) == -1){
        close(fd);
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }

    close(fd);
}

void leerArchivoDesdeHasta(char* FILEPATH,char* bloque,int byteInicio,int byteFinal){
    int i,j,fd;
    char *buffer;
    struct stat fileInfo = {0};
    char* data;

    j=0;
    fd=open(FILEPATH, O_RDWR, (mode_t)0600);
    buffer = malloc(tamArchivo(FILEPATH));
    data = mmap(0, tamArchivo(FILEPATH), PROT_READ, MAP_SHARED, fd, 0);

    if (data == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }
    for (i = byteInicio; i < byteFinal; i++)
    {
		if(NULL!=data[i]){
			//printf("%c",data[i]);
			buffer[j]=data[i];
			j++;
		}
    }
    //buffer[j]='\0';
    for(i=0;i<j;i++){
    	bloque[i]=buffer[i];
    }
    //strcpy(bloque,buffer);

    if (munmap(data, tamArchivo(FILEPATH)) == -1)
    {
        close(fd);
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }

    // Un-mmaping doesn't close the file, so we still need to do that.
    close(fd);
    free(buffer);
}

void escribirArchivo(char* FILEPATH,char* datos){
   int fd,i;
   struct stat mystat;
   size_t FILESIZE,textSize;
   char *data;  /* mmapped array of chars */

   fd = open(FILEPATH, O_RDWR | O_CREAT, S_IRWXU);
   if (fd == -1) {
	   perror("Error opening file for writing");
		exit(EXIT_FAILURE);
   }
   if(stat(FILEPATH,&mystat)<0){
	   perror("fstat");
	   close(fd);
	   exit(1);
   }

   FILESIZE=mystat.st_size;
   textSize=strlen(datos);

   if ((lseek(fd, textSize-1, SEEK_SET))== -1) {
	   close(fd);
	   perror("Error calling lseek() to 'stretch' the file");
	   exit(EXIT_FAILURE);
   }

   if ((write(fd,"", 1)) != 1) {
	   close(fd);
	   perror("Error writing last byte of the file");
	   exit(EXIT_FAILURE);
   }
   data = mmap(0, textSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SEEK_SET);

   if (data == MAP_FAILED) {
	   close(fd);
	   perror("Error mmapping the file");
	   exit(EXIT_FAILURE);
   }

   memcpy(data,datos,textSize);

   if (munmap(data, textSize) == -1) {
	   perror("Error un-mmapping the file");
	   close(fd);
	   exit(1);
   }
   close(fd);
}

void appendArchivo(char* FILEPATH,char* datos){
	FILE* archivo = fopen(FILEPATH, "a");
	fwrite(datos, strlen(datos), 1, archivo);
	fclose(archivo);
}

void escribirArchivoBinario(char* FILEPATH,char* datos,int tamBloque){
	   int fd,i;
	   struct stat mystat;
	   size_t FILESIZE,textSize;
	   char *data;  /* mmapped array of chars */
	   size_t punteroArchivo;
	   fd = open(FILEPATH, O_RDWR | O_CREAT, S_IRWXU);
	   if (fd == -1) {
		   perror("Error opening file for writing");
			exit(EXIT_FAILURE);
	   }
	   if(stat(FILEPATH,&mystat)<0){
		   perror("fstat");
		   close(fd);
		   exit(1);
	   }
	   FILESIZE=mystat.st_size;
	   textSize=strlen(datos);
	   punteroArchivo=(FILESIZE/tamBloque)+tamBloque;
	   if ((lseek(fd, punteroArchivo+1, SEEK_SET))== -1) {
		   close(fd);
		   perror("Error calling lseek() to 'stretch' the file");
		   exit(EXIT_FAILURE);
	   }
	   if ((write(fd,"", 1)) != 1) {
		   close(fd);
		   perror("Error writing last byte of the file");
		   exit(EXIT_FAILURE);
	   }


	   data = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SEEK_SET);

	   if (data == MAP_FAILED) {
		   close(fd);
		   perror("Error mmapping the file");
		   exit(EXIT_FAILURE);
	   }

	   for(i=0;i<strlen(datos);i++){
		   data[i]=datos[i];
	   }

	   if (munmap(data, textSize) == -1) {
		   perror("Error un-mmapping the file");
		   close(fd);
		   exit(1);
	   }
	   close(fd);
}

void recibirArchivoM(int *sock,struct sockaddr_in *miDireccion,char* nombreServidor){
	struct sockaddr_in direccionCliente; // Direccion del cliente
    int bytesRecibidos,addrlen,cliente,ret; // Bytes recividos por el cliente
    paqueteDatos *datosRecibidos=malloc(sizeof(paqueteDatos));
	char nuevoNombre[100]; // Buffer para guardar los datos que envia el cliente

	void *buffer= malloc (sizeof(u_int32_t));
	void *tmp_buffer;
	char ruta[100]="/home/utnso/git/tp-2017-2c-Rafaga-de-amor-/Worker/Archivos recibidos/";

    if (listen(*sock, 10) == -1) { // Pone al servidor a la espera de conexiones entrantes
        perror("listen");
        exit(1);
    }
	cliente=accept(*sock,&direccionCliente,&addrlen);
	if(cliente==-1){
		perror("accept");
	}else{
		if(send(cliente,"[Worker]:Recibiendo archivo\n",30,0)==-1){
			perror("send");
		}
	}
	printf("Recibiendo archivo\n");
	if ((bytesRecibidos=recv(cliente, buffer, sizeof(u_int32_t), 0)) == -1) { // Recibo el primer mensaje que indica el tamaño del archivo
		perror("recv");
		exit(1);
	}else{
		datosRecibidos->tamContenido=*((u_int32_t*)buffer);
		//printf("%d \n",*(u_int32_t*)buffer);

	}
	tmp_buffer=realloc(buffer,(sizeof(u_int32_t)+*(u_int32_t*)buffer+sizeof(datosRecibidos->nombre)));
	if (tmp_buffer== NULL) { //Asigno mas memoria al buffer para recibir el contenido del archivo
		printf("Error realloc");
		exit(1);
	}
	else {
		/* Reasignación exitosa. Asignar memoria a ptr */
		buffer = tmp_buffer;
	}
	if ((bytesRecibidos=recv(cliente, buffer+sizeof(u_int32_t), *((u_int32_t*)buffer), 0)) == -1) { // Recibo el segundo mensaje con el contenido del archivo
		perror("recv");
		exit(1);
	}else{
		//printf("%s \n",buffer+sizeof(u_int32_t));
		datosRecibidos->contenido=malloc(datosRecibidos->tamContenido);
		strcpy(datosRecibidos->contenido,buffer+sizeof(u_int32_t));
	}
	if ((bytesRecibidos=recv(cliente, buffer+sizeof(u_int32_t)+*((u_int32_t*)buffer), sizeof(datosRecibidos->nombre), 0)) == -1) { // Recibo el ultimo mensaje con el nombre del archivo
		perror("recv");
		exit(1);
	}else{
		//printf("%s \n",buffer+sizeof(u_int32_t)+*((u_int32_t*)buffer));
		strcpy(datosRecibidos->nombre,buffer+sizeof(u_int32_t)+*((u_int32_t*)buffer));
	}
	//escribirArchivo("/home/utnso/git/tp-2017-2c-Rafaga-de-amor-/Worker/Archivos recibidos/nuevoArchivoRecibido.txt",datosRecibidos->contenido);
	strcpy(nuevoNombre,datosRecibidos->nombre);
	strcat(ruta,nuevoNombre);

	free(datosRecibidos->contenido);
	free(datosRecibidos);
	free(buffer);
	close(sock);
	ret=rename("/home/utnso/git/tp-2017-2c-Rafaga-de-amor-/Worker/Archivos recibidos/nuevoArchivoRecibido.txt",ruta); //Renombro el archivo

	if(ret!=0){
		printf("No se pudo renombrar el archivo \n");
	}
}

void enviarArchivoM(int sock,char* path){
	char ruta[100];
	char *pc;
	void *buffer;

	paqueteDatos *datosEnvio=malloc(sizeof(paqueteDatos));
	datosEnvio->tamContenido=tamArchivo(path);
	datosEnvio->contenido=malloc(datosEnvio->tamContenido);
	leerArchivo(path,datosEnvio->contenido);
	strcpy(ruta,path);
	pc=strtok(ruta,"/");
	while( pc != NULL ){ //Agarra el ultimo token "/" y copia lo que hay delante de el (nombre del archivo) y lo guarda en nombreArchivo
		strcpy(datosEnvio->nombre,pc);
		pc = strtok(NULL, "/");
	}
	buffer=malloc(sizeof(u_int32_t)+datosEnvio->tamContenido+sizeof(datosEnvio->nombre));
	memcpy(buffer,&datosEnvio->tamContenido,sizeof(u_int32_t));
	memcpy(buffer+sizeof(u_int32_t) ,datosEnvio->contenido,datosEnvio->tamContenido);
	memcpy(buffer+sizeof(u_int32_t)+datosEnvio->tamContenido,datosEnvio->nombre,sizeof(datosEnvio->nombre));
	if(send(sock,buffer,sizeof(u_int32_t)+datosEnvio->tamContenido+sizeof(datosEnvio->nombre),0) == -1){
		printf("Error al enviar linea");
	}
	free(datosEnvio->contenido);
	free(datosEnvio);
	free(buffer);
	close(sock);
}

int existeArchivo(char* FILEPATH){
	int fd,result=-1;
	fd=open(FILEPATH, O_RDONLY,0);
	if(fd==-1){
		result = 1;//El archivo no existe
	}
	else{
		result = 0;//El archivo ya existe
		close(fd);
	}

	return result;
}

int tamArchivo(char* path){
    int fd;
    struct stat fileInfo = {0};
    fd=open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }
    if (fstat(fd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        return -1;
    }
	return fileInfo.st_size;
	close(fd);
}

void limpiarArchivo(char* pathArchivo){
	   int fd;
	   struct stat mystat;

	   size_t FILESIZE;
	   void *data;  /* mmapped array of chars */
	   fd = open(pathArchivo, O_RDWR , S_IRWXU);
	   if (fd == -1) {
		   perror("Error opening file for writing");
			exit(EXIT_FAILURE);
	   }
	   if(stat(pathArchivo,&mystat)<0){
		   perror("fstat");
		   close(fd);
		   exit(1);
	   }

	   FILESIZE=mystat.st_size;

	   data = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);

	   if (data == MAP_FAILED) {
		   close(fd);
		   perror("Error mmapping the file");
		   exit(EXIT_FAILURE);
	   }
	   memset(data,'\0',FILESIZE);
	   if (munmap(data, FILESIZE) == -1) {
		   perror("Error un-mmapping the file");
		   close(fd);
		   exit(1);
	   }
}

char* obtenerNombreArchivo(char* pathArchivoLocal){
	char **split;
	char *nombreArchivo=string_new();
	int i=0;
	split = string_split(pathArchivoLocal,"/");
	while(NULL!=split[i]){ //Agarro el ultimo nombre de directorio
		i++;
	}
	i--;
	string_append(&nombreArchivo,split[i]);
	free(split);
	return nombreArchivo;
}
