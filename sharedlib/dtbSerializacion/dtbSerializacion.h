#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>

typedef struct DT_Block {
	int ID_GDT;
	char Escriptorio[256]; //El valor despues hay que verlo bien cuando nos den un ejemplo de Script
	int PC;
	int Flag_EstadoGDT;
	t_list *tablaArchivosAbiertos;
} DTB;

enum { lenMaxNombre = 128 };

typedef struct datosArchivo {
	char nombreArchivo[lenMaxNombre];
	long dirMemoria;
} datosArchivo;

void myTrim(char *aDonde,char *contenidoACortar);


void recibirDTB(int socket);
void imprimirDTB(DTB *miDTB);
char* DTBStruct2String(DTB *miDTB);
DTB* DTBString2Struct (char *miStringDTB);
