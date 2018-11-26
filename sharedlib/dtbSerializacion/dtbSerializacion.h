#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>

typedef struct DT_Block {
	int ID_GDT;
	char Escriptorio[256]; //El valor despues hay que verlo bien cuando nos den un ejemplo de Script
	int PC;
	int Flag_GDTInicializado;
	int ejecutoSuUltimaSentencia; // 1 si 0 no
	t_list *tablaArchivosAbiertos;
} DTB;

enum { lenMaxNombre = 256 };

enum { MOT_QUANTUM, MOT_BLOQUEO, MOT_FINALIZO, MOT_ERROR, ACC_WAIT, ACC_SIGNAL};

enum {DESCONEXION_SAFA,PREGUNTAR_DESCONEXION_CPU,EJECUCION_NORMAL};

typedef struct datosArchivo {
	char pathArchivo[lenMaxNombre];
	int fileID;
} datosArchivo;


void myTrim(char *aDonde,char *contenidoACortar);


DTB* recibirDTB(int socket);
void imprimirDTB(DTB *miDTB);
char* DTBStruct2String(DTB *miDTB);
DTB* DTBString2Struct (char *miStringDTB);
