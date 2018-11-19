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
	if(validarPathArchivoFS(pathArchivoFS)==0){
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

char* obtenerDatos(char* pathArchivoFS,int offset,int size){
	if(validarPathArchivoFS(pathArchivoFS)==0){
		int tamArchivo = obtenerTamArchivoFS(pathArchivoFS);
		if(size>tamArchivo)
			size=tamArchivo;
		if(offset > tamArchivo)
			return "-1";
		if((offset+size)>tamArchivo)
			size=tamArchivo-offset;
		char* datos=obtenerArchivoFS(pathArchivoFS);
		char* datosAux=malloc(size+1);
		memset(datosAux,'\0',size+1);
		memcpy(datosAux,datos+offset,size);
		free(datos);
		return datosAux;
	}

	return "-1";
}

char* obtenerDatosOld(char* pathArchivoFS,int offset,int size){
	if(validarPathArchivoFS(pathArchivoFS)==0){
		char* datos;
		char** bloquesArchivo=obtenerBloquesArchivoFS(pathArchivoFS);
		int tamArchivo = obtenerTamArchivoFS(pathArchivoFS);
		int i=offset/tamBloque;
		datos=string_new();
		if(offset>tamArchivo){
			printf("Offset mayor al tamaño del archivo \n");
			liberarSplit(bloquesArchivo);
			return datos;
		}
		if(size>tamBloque){
			if(size>(tamArchivo-offset)){
				size=tamArchivo-offset;
			}
			char* datosAux = leerBloqueDesdeHasta(bloquesArchivo[i],(offset%tamBloque),tamBloque);
			string_append(&datos,datosAux);
			free(datosAux);
			size-=tamBloque-(offset%tamBloque);
			i++;
			while(size>0){
				if(size>tamBloque){
					char* datosAux = leerBloqueDesdeHasta(bloquesArchivo[i],0,tamBloque);
					string_append(&datos,datosAux);
					free(datosAux);
					size-=tamBloque;
					i++;
				}else{
					char* datosAux = leerBloqueDesdeHasta(bloquesArchivo[i],0,size);
					string_append(&datos,datosAux);
					printf("size %d",size);
					size=0;
					free(datosAux);
				}
			}
		}else{
			if(((offset%tamBloque)+size)>tamBloque){
				char* datosAux = leerBloqueDesdeHasta(bloquesArchivo[i],(offset%tamBloque),tamBloque);
				string_append(&datos,datosAux);
				free(datosAux);
				size-=tamBloque-(offset%tamBloque);
				char* datosSig = leerBloqueDesdeHasta(bloquesArchivo[i+1],0,size);
				string_append(&datos,datosSig);
				free(datosSig);
			}else{
				char* datosAux = leerBloqueDesdeHasta(bloquesArchivo[i],(offset%tamBloque),size+(offset%tamBloque));
				string_append(&datos,datosAux);
				free(datosAux);
			}

		}
		liberarSplit(bloquesArchivo);
		return datos;
	}

	return "-1";
}

int guardarDatos(char* pathArchivoFS,int offset, int size,char* buffer){
	if(validarPathArchivoFS(pathArchivoFS)==0){
		char** bloquesArchivo=obtenerBloquesArchivoFS(pathArchivoFS);
		char* datosAux=malloc(tamBloque+1);//Datos a escribir
		int tamArchivo = obtenerTamArchivoFS(pathArchivoFS);
		int i=offset/tamBloque;
		int desplazamiento=0;
		if(offset>tamArchivo){
			printf("Offset mayor al tamaño del archivo \n");
			liberarSplit(bloquesArchivo);
			return -1;
		}

		if(size>tamBloque){
			if(size>(tamArchivo-offset)){
				size=tamArchivo-offset;
			}
			memset(datosAux,'\0',tamBloque+1);
			memcpy(datosAux,buffer,(tamBloque-(offset%tamBloque)));
			escribirBloqueDesde(bloquesArchivo[i],(offset%tamBloque),datosAux);
			desplazamiento+=(tamBloque-(offset%tamBloque));
			size-=tamBloque-(offset%tamBloque);
			i++;
			while(size>0){
				if(size>tamBloque){
					/*char* datosAux = leerBloqueDesdeHasta(bloquesArchivo[i],0,tamBloque);
					string_append(&datos,datosAux);
					free(datosAux);
					size-=50;
					i++;*/
					memset(datosAux,'\0',tamBloque+1);
					memcpy(datosAux,buffer+desplazamiento,tamBloque);
					escribirBloqueDesde(bloquesArchivo[i],0,datosAux);
					desplazamiento+=tamBloque;
					size-=tamBloque;
					i++;

				}else{
					/*char* datosAux = leerBloqueDesdeHasta(bloquesArchivo[i],0,size);
					string_append(&datos,datosAux);
					size=0;
					free(datosAux);*/
					memset(datosAux,'\0',tamBloque+1);
					memcpy(datosAux,buffer+desplazamiento,size);
					escribirBloqueDesde(bloquesArchivo[i],0,datosAux);
					desplazamiento+=size;
					size=0;
					i++;
				}
			}
		}else{
			if(((offset%tamBloque)+size)>tamBloque){

				memset(datosAux,'\0',tamBloque+1);
				memcpy(datosAux,buffer,(tamBloque-(offset%tamBloque)));
				escribirBloqueDesde(bloquesArchivo[i],(offset%tamBloque),datosAux);
				size-=tamBloque-(offset%tamBloque);
				memset(datosAux,'\0',tamBloque+1);
				memcpy(datosAux,buffer+(tamBloque-(offset%tamBloque)),size);
				escribirBloqueDesde(bloquesArchivo[i+1],0,datosAux);
			}else{
				escribirBloqueDesde(bloquesArchivo[i],(offset%tamBloque),buffer);
			}

		}

		free(datosAux);
		liberarSplit(bloquesArchivo);
		return 0;
	}

	return -1;
}

char* obtenerArchivo(char* pathArchivoFS){ //pathFSArchivo-> Path del archivo en el FileSystem Fifa, pathABSArchivo-> Path absoluto del archivo en filesystem Unix

	return obtenerArchivoFS(pathArchivoFS);
}
