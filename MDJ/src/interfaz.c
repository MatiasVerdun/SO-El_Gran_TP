/*
 * interfaz.c
 *
 *  Created on: 6 sep. 2018
 *      Author: utnso
 */
#include "interfaz.h"
#include "filesystemFIFA.h"


int validarArchivo(char *path){
	return existeArchivoFS(path);
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

char* obtenerArchivo(char* pathFSArchivo){ //pathFSArchivo-> Path del archivo en el FileSystem Fifa, pathABSArchivo-> Path absoluto del archivo en filesystem Unix

	return obtenerArchivoFS(pathFSArchivo);
}
