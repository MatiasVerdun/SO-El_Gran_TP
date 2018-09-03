#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#ifndef CONEXIONES_MYSOCKETS_H_
#define CONEXIONES_MYSOCKETS_H_


int myEnlazarCliente(int *sock,char* IP_CLIENTE, int PUERTO_CLIENTE);
int my_EnlazarCliente(int *sock,char* IP_CLIENTE, int PUERTO_CLIENTE,int useHandshake);

int myEnlazarServidor(int *sock_Servidor,struct sockaddr_in *miDireccion,char* IP_SERVIDOR, int PUERTO_SERVIDOR);
int myAtenderCliente(int *sock_Servidor, char *nombreServidor, char *nombreCliente, int *sock_Cliente);
int my_AtenderCliente(int *sock_Servidor, char *nombreServidor, char *nombreCliente, int *sock_Cliente, int useHandshake);
int myAtenderClientesEnHilos(int *sock_Servidor, char *nombreServidor, char *nombreCliente,void (*funcHiloCliente)(int*));
int my_AtenderClientesEnHilos(int *sock_Servidor, char *nombreServidor, char *nombreCliente,void (*funcHiloCliente)(int*),int useHandshake);

int myRecibirDatosFijos(int descriptorSocket, const void* buffer, const unsigned int bytesPorRecibir);
int myEnviarDatosFijos(int descriptorSocket, const void* buffer, const unsigned int bytesPorEnviar);

void* getConfig(char* token,char* nombreConfig,int tipo);

#endif /* CONEXIONES_MYSOCKETS_H_ */