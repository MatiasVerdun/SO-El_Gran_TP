#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include "../conexiones/mySockets.h"

int verificarCarpeta(char* path);
void escribir(char* FILEPATH,char* datos);
void leerArchivo(char* FILEPATH,char* buffer);
void escribirArchivo(char* FILEPATH,char* datos);
void appendArchivo(char* FILEPATH,char* datos);
int existeArchivo(char* FILEPATH);
void recibirArchivoM(int *sock,struct sockaddr_in *miDireccion,char* nombreServidor);
void enviarArchivoM(int sock,char* path);
int tamArchivo(char* path);
void limpiarArchivo(char* pathArchivo);
char* obtenerNombreArchivo(char* pathArchivoLocal);
void leerArchivoDesdeHasta(char* FILEPATH,char* bloque,int byteInicio,int byteFinal);
