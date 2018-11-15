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

int crearArchivo(char* pathArchivoFS,u_int32_t filesize){
	int cantidadBloques=0;
	int offsetUltimoBloque=0;
	char *metadataArchivo;
	cantidadBloques=filesize/tamBloque;
	if(filesize%tamBloque!=0)
		cantidadBloques++;
	metadataArchivo=string_from_format("TAMANIO=%d\nBLOQUES=[",filesize);
	if(validarPathArchivoFS(pathArchivoFS)==0){//TODO crear estructuras administrativas archivo
		//printf("Cantidad de bloques necesarios %d\n",cantidadBloques);
		if(getCantBloquesLibres()<cantidadBloques){
			printf("Bloques insuficientes para guardar los datos solicitados\n");
			return -1;
		}
		offsetUltimoBloque=filesize%tamBloque;
		for (int j=0;j<cantidadBloques;j++){//Para recorrer los bloques
			char* proximoBloqueLibre=string_itoa((int)getNBloqueLibre());
			string_append(&metadataArchivo,proximoBloqueLibre);
			if(j==(cantidadBloques-1)){
				char* datosVacios= malloc(offsetUltimoBloque+1);
				memset(datosVacios,'\0',offsetUltimoBloque+1);
				memset(datosVacios,'\n',offsetUltimoBloque);
				escribirBloque(proximoBloqueLibre,datosVacios);
				free(datosVacios);
			}else{
				char* datosVacios = malloc(tamBloque+1);
				memset(datosVacios,'\0',tamBloque+1);
				memset(datosVacios,'\n',tamBloque);
				escribirBloque(proximoBloqueLibre,datosVacios);
				string_append(&metadataArchivo,",");
				free(datosVacios);
			}
			free(proximoBloqueLibre);
		}
		string_append(&metadataArchivo,"]");
		escribirMetadataArchivo(metadataArchivo,pathArchivoFS);
		free(metadataArchivo);
	}else{
		printf("La ruta especificada no es valida\n");
		return -1;
	}
	return 0;
}

int borrarArchivo(char* pathArchivoFS){
	if(validarPathArchivoFS(pathArchivoFS)==0){
		t_config *metadataArchivo;
		int i=0;
		char** bloques;
		char* puntoMontaje= string_from_format("%s",(char*)getConfigR("PUNTO_MONTAJE",0,configMDJ));
		char* pathABSarchivo=string_from_format("%sArchivos/%s", puntoMontaje,pathArchivoFS);

		metadataArchivo=config_create(pathABSarchivo);
		bloques=config_get_array_value(metadataArchivo, "BLOQUES");
		while(bloques[i]!=NULL){
			limpiarBloque(bloques[i]);
			i++;
		}
		remove(pathABSarchivo);
		config_destroy(metadataArchivo);
		liberarSplit(bloques);
		free(puntoMontaje);
		free(pathABSarchivo);
	}else{
		printf("La ruta especificada no es valida\n");
		return -1;
	}

	return 0;
}

char* obtenerArchivo(char* pathFSArchivo){ //pathFSArchivo-> Path del archivo en el FileSystem Fifa, pathABSArchivo-> Path absoluto del archivo en filesystem Unix

	return obtenerArchivoFS(pathFSArchivo);
}
