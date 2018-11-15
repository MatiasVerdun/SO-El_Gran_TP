/*
 * filesystemFIFA.h
 *
 *  Created on: 7 sep. 2018
 *      Author: utnso
 */


#ifndef FILESYSTEMFIFA_H_
#define FILESYSTEMFIFA_H_
#include <archivos/archivos.h>
#include <console/myConsole.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/config.h>
#include <stdio.h>
#include <dirent.h>

#define PATHD "/home/utnso/tp-2018-2c-smlc/MDJ/Metadata/directorios.dat"
#define PATHCONFIGMDJ "/home/utnso/tp-2018-2c-smlc/Config/MDJ.txt"
#define tamDirectorio 256
#define tamMaxDirectorios tamDirectorio*100

typedef struct tablaDirectory {
	int index;
	char nombre[255];
	int padre;
} tableDirectory;

t_bitarray *bitmap;
t_config *configMDJ;
size_t tamBloque;
size_t cantBloques;

void escribirMetadataArchivo(char* metadata,char* pathArchivoFS);
void setBloqueLibre(int index);
void setBloqueOcupado(int index);
int getNBloqueLibre();
int getCantBloquesLibres();
void pruebaBitmap(int tipo);
void guardarBitmap();
void cargarBitmap();
void mostrarBitmap();
int validarPathArchivoFS(char* pathArchivoFS);
void cargarFS();
int existeCarpetaFS(char* pathCarpetaFS);
int existeArchivoFS(char* pathArchivoFS);
void escribirBloque(char* nroBloque,char* datos);
void limpiarBloque(char* nroBloque);
void leerArchivoMDJ(char* pathFSArchivo);
char* leerBloqueDesdeHasta(char* nroBloque,int offset,int size);
char* leerBloque(char* nroBloque);
char** obtenerBloquesArchivoFS(char* pathArchivoFS);
int obtenerTamArchivoFS(char* pathFSArchivo);
char* obtenerArchivoFS(char* pathFSArchivo);
void escribirDirectorioIndice(char* datos,int indice);
void actualizarArchivoDirectorio(struct tablaDirectory *t_directorios);
int obtenerPadreDir(char* nombreDir);
int obtenerIndiceDir(char* nombreDir);
int crearDirectorio(struct tablaDirectory *t_directorios,char* pathDir);
void crearDirectorioRoot(struct tablaDirectory *t_directorios);
void inicializarTdir(struct tablaDirectory *t_directorios);
void inicializarDir(struct tablaDirectory *t_directorios);
void crearArchivoDirectorio();
void crearMetadata();
void listarPadresDir(struct tablaDirectory *t_directorios,int i);
void listarDirectorios(struct tablaDirectory *t_directorios,int i,int nivel);
int validarPathDir(char* pathDir);
int borrarDirectorio(struct tablaDirectory *t_directorios,char* pathDir);
int leerArchivoDirectorio(struct tablaDirectory *t_directorios,int numeroDirectorio);
void cargarStructDirectorio(struct tablaDirectory *t_directorios);
int array_length(void* array);
#endif /* FILESYSTEMFIFA_H_ */
