/*
 * interfaz.h
 *
 *  Created on: 6 sep. 2018
 *      Author: utnso
 */
#include <archivos/archivos.h>
#include <commons/string.h>
#ifndef INTERFAZ_H_
#define INTERFAZ_H_

char* obtenerArchivo(char* pathFSArchivo);
int validarArchivo(char* path);
int crearArchivo(char* pathArchivoFS,u_int32_t size);
int borrarArchivo(char* pathArchivoFS);
char* obtenerDatos(char* path,int offset,int size);
int guardarDatos(char* pathArchivoFS,int offset, int size,char* buffer);
#endif /* INTERFAZ_H_ */

