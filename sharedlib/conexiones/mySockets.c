#include "mySockets.h"
#include "../console/myConsole.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

int myEnlazarCliente(int *sock,char* IP_CLIENTE, int PUERTO_CLIENTE){
	return my_EnlazarCliente(sock, IP_CLIENTE, PUERTO_CLIENTE, 1);						// Por defecto usa Handshake
}

int my_EnlazarCliente(int *sock,char* IP_CLIENTE, int PUERTO_CLIENTE,int useHandshake){
	struct sockaddr_in direccionServidor;
    int numbytes;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(IP_CLIENTE);
	direccionServidor.sin_port = htons(PUERTO_CLIENTE);

	*sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(*sock, (void*) &direccionServidor, sizeof(direccionServidor)) != 0) {
		perror("myEnlazarServidor - No se pudo conectar");
		return 1;
	}
	if (useHandshake!=0)
	{
	    char handshake[50];
//	    strset(handshake,'\0');

		if ((numbytes=recv(*sock, handshake, sizeof(handshake), 0)) == -1) {
			perror("myEnlazarServidor -  No pudo hacer recv de handshake");
			return 1;
		}
	    handshake[numbytes]='\0';
	    myPuts("%s\n",handshake);
    }

	return 0;
}

int myEnlazarServidor(int *sock_Servidor,struct sockaddr_in *miDireccion,char* IP_SERVIDOR, int PUERTO_SERVIDOR){
	struct sockaddr_in direccionServidor; // Direccion del servidor
	int activado = 1;
	direccionServidor.sin_family = AF_INET; // Ordenación de máquina
	direccionServidor.sin_addr.s_addr = inet_addr(IP_SERVIDOR);// Usa mi dirección IP
	direccionServidor.sin_port = htons(PUERTO_SERVIDOR);// Short, Ordenación de la red
	memset(&(direccionServidor.sin_zero), '\0', 8); // Poner a cero el resto de la estructura
	*sock_Servidor = socket(AF_INET, SOCK_STREAM, 0);
	if (*sock_Servidor==-1){
        perror("myEnlazarServidor - Error al crear el socket.");
		return 1;
	}

    if (setsockopt(*sock_Servidor, SOL_SOCKET, SO_REUSEADDR, &activado,sizeof(activado)) == -1) { // Obviar el mensaje "address already in use" (la dirección ya se está usando)
        perror("myEnlazarServidor - Error al setear opciones de Socket 'setsockopt'.");
        return 1;
    }
	if (bind(*sock_Servidor, (struct sockaddr*) &direccionServidor, sizeof(direccionServidor)) != 0) { // Asocia el socket con un puerto de tu maquina local
        perror("myEnlazarServidor - Error al realizar el 'bind'.");
		return 1;
	}

	*miDireccion=direccionServidor;
	return 0;
}

int myAtenderCliente(int *sock_Servidor, char *nombreServidor, char *nombreCliente, int *sock_Cliente){
	return my_AtenderCliente(sock_Servidor, nombreServidor, nombreCliente, sock_Cliente, 1);		// Por defecto usa Handshake
}

int my_AtenderCliente(int *sock_Servidor, char *nombreServidor, char *nombreCliente, int *sock_Cliente, int useHandshake){
	struct sockaddr_in direccionCliente;
	int socket_cliente;
	int *new_sock;

	char mensajeAlCliente[50]="Te conectaste con ";
	strcat(mensajeAlCliente,nombreServidor);

    if (listen(*sock_Servidor, 1) == -1) { // Pone al servidor a la espera de conexiones entrantes
        perror("myAtenderClientes - Error en 'listen'");
        return 1;
    }

	myPuts("Servidor: %s Arrancado. Esperando la conexion del %s ... \n",nombreServidor,nombreCliente);

	int addrlen=sizeof(direccionCliente);

	socket_cliente = accept(*sock_Servidor,(struct sockaddr*)&direccionCliente,(socklen_t*)&addrlen);

    	if (socket_cliente < 0)
    	{
            perror("myAtenderClientes - Error en 'accept'");
            return 1;
        }

		myPuts("Recibí una conexión de un proceso %s!!\n", nombreCliente);

		if (useHandshake!=0)
    	{
			if(send(socket_cliente,mensajeAlCliente,strlen(mensajeAlCliente),0)==-1){ //PASAR POR PARAMETRO EL NOMBRE DEL SERVIDOR
				perror("myAtenderClientes - Error al enviar Msg de Echo");
			}
		}

		//new_sock = malloc(1);
		*sock_Cliente = socket_cliente;


	return 0;
}

int myAtenderClientesEnHilos(int *sock_Servidor, char *nombreServidor, char *nombreCliente, void (*funcHiloCliente)(int*)){
	return my_AtenderClientesEnHilos(sock_Servidor, nombreServidor, nombreCliente, funcHiloCliente, 1);		// Por defecto usa Handshake
}

//TODO error comun es que devuelven entero y algunos caminos de la funcion no tienen retorno, como al final
int my_AtenderClientesEnHilos(int *sock_Servidor, char *nombreServidor, char *nombreCliente, void (*funcHiloCliente)(int*),int useHandshake){
	struct sockaddr_in direccionCliente;
	int sock_cliente=-1;
	int *new_sock=-1;

	char mensajeAlCliente[50]="Te conectaste con ";
	strcat(mensajeAlCliente,nombreServidor);

    if (listen(*sock_Servidor, 10) == -1) { // Pone al servidor a la espera de conexiones entrantes
        perror("myAtenderClientes - Error en 'listen'");
        return 1;
    }

	myPuts("\rServidor: %s Arrancado. Esperando por conexiones de %s ...\n",nombreServidor,nombreCliente);

	int addrlen=sizeof(direccionCliente);
	while((sock_cliente=accept(*sock_Servidor,(struct sockaddr*)&direccionCliente,(socklen_t*)&addrlen)))
	{
    	if (sock_cliente < 0)
    	{
            perror("myAtenderClientes - Error en 'accept'");
            return 1;
        }

		myPuts("Recibí una conexión en %d de un proceso %s!!\n", sock_cliente, nombreCliente);

		if (useHandshake!=0)
    	{
			if(send(sock_cliente,mensajeAlCliente,strlen(mensajeAlCliente),0)==-1){ //PASAR POR PARAMETRO EL NOMBRE DEL SERVIDOR
				perror("myAtenderClientes - Error al enviar Msg de Echo");
			}
		}

		pthread_t hiloCliente;
		new_sock = malloc(4);
		*new_sock = sock_cliente;

		if( pthread_create( &hiloCliente, NULL,  funcHiloCliente , (int*) new_sock) < 0)
		{
            perror("myAtenderClientes - Error en 1er. 'send'");
			perror("could not create thread");
			return 1;
		}
		//return sock_cliente;
		//return 0;
	}
	return 0;
}

int myRecibirDatosFijos(int descriptorSocket, const void* buffer, const unsigned int bytesPorRecibir){
	int retorno=-1;
	int bytesRecibidos=0;


	while (bytesRecibidos < (int)bytesPorRecibir) {
	   retorno = recv(descriptorSocket, (void*)(buffer+bytesRecibidos), bytesPorRecibir-bytesRecibidos, 0);
#ifdef TRACE
	   myPuts("Bytes Recibidos: %d / %d \n", retorno, bytesPorRecibir);
#endif
	   //Controlo Errores
	   if( retorno <= 0 ) {
		  //myPuts("Error al recibir Datos (se corto el Paquete Recibido), solo se recibieron %d bytes de los %d bytes totales por recibir\n", bytesRecibidos, (int)bytesPorRecibir);
		  bytesRecibidos = retorno;
		  return 1;
	   }
	   //Si no hay problemas, sigo acumulando bytesEnviados
	   bytesRecibidos+= retorno;
	}

	return 0;
}

int myEnviarDatosFijos(int descriptorSocket, const void* buffer, const unsigned int bytesPorEnviar){
	int retorno;
	int bytesEnviados = 0;

//	int strlenBuffer = strlen(buffer);


/*	if(strlenBuffer != (int)bytesPorEnviar)
		printf("myEnviarDatosFijos - No coinciden el largo del buffer con la cantidad de bytes a enviar. strlen(buffer) = %d, bytesPorEnviar = %d\n", strlenBuffer, (int)bytesPorEnviar);
*/

	while (bytesEnviados < (int)bytesPorEnviar) {
	   retorno = send(descriptorSocket, (void*)(buffer+bytesEnviados), bytesPorEnviar-bytesEnviados, 0);
	   //printf("Bytes enviados: %d / %d \n", retorno, bytesPorEnviar);

	   //Controlo Errores
	   if( retorno <= 0 ) {
		  myPuts("Error al enviar Datos (se corto el Paquete Enviado), solo se enviaron %d bytes de los %d bytes totales por enviar\n", bytesEnviados, (int)bytesPorEnviar);
		  bytesEnviados = retorno;
		  break;
	   }
	   //Si no hay problemas, sigo acumulando bytesEnviados
		bytesEnviados += retorno;
	}

	return 0;
}

int gestionarDesconexion(int socket,char* nombreProceso){
	char buff[3];
	int rta=0;
	rta=recv((int)socket,buff,sizeof(buff),0);//
	while(1){
		if(rta<=0){
			myPuts(RED "Se desconecto el proceso %s" COLOR_RESET "\n",nombreProceso);
			return 1;
		}
	}
	return 0;
}
