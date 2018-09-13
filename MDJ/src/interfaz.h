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

int validarArchivo(char* path);
int crearArchivo(char* path);
int obtenerDatos(char* path,int offset,int size);
int guardarDatos(char* path,int offset,int size);
#endif /* INTERFAZ_H_ */

