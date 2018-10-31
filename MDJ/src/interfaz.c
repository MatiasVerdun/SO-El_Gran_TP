/*
 * interfaz.c
 *
 *  Created on: 6 sep. 2018
 *      Author: utnso
 */
#include "interfaz.h"

int validarArchivo(char *path){
	int existe=1;
	FILE *fp = fopen(path,"r");
	if(fp) existe=0;
	return existe;
}

int crearArchivo(char* path){
	int resultado=1;
	FILE *fp = fopen(path, "ab+");
	if (fp) resultado=0;
	return resultado;
}


