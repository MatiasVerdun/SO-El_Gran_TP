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

int crearArchivo(char* path,u_int32_t size){
	FILE *fp = fopen(path, "ab+");
	if (!fp) return 1;
	for(int i=0;i<size;i++){
		fputc('\n',fp);
	}
    fclose(fp);
	return 0;
}


