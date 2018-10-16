#ifndef PARSER_PARSER_H_
#define PARSER_PARSER_H_

	#include <stdlib.h>
	#include <stdio.h>
	#include <stdbool.h>
	#include <commons/string.h>

	typedef struct {
		bool valido;
		enum {
			ABRIR,
			CONCENTRAR,
			ASIGNAR,
			WAIT,
			SIGNAL,
			FLUSH,
			CLOSE,
			CREAR,
			BORRAR
		} keyword;
		union {
			struct {
				char* param1;
			} ABRIR;
			struct {
				char* param1;
				int param2;
				char* param3;
			} ASIGNAR;
			struct {
				char* param1;
			} WAIT;
			struct {
				char* param1;
			} SIGNAL;
			struct {
				char* param1;
			} FLUSH;
			struct {
				char* param1;
			} CLOSE;
			struct {
				char* param1;
				int param2;
			} CREAR;
			struct {
				char* param1;
			} BORRAR;
		} argumentos;
		char** _raw; //Para uso de la liberación
	} t_parser_operacion;

	/**
	* @NAME: parse
	* @DESC: interpreta una linea de un archivo ESI y
	*		 genera una estructura con el operador interpretado
	* @PARAMS:
	* 		line - Una linea de un archivo ESI
	*/
	t_parser_operacion parse(char* line);


#endif /* PARSER_PARSER_H_ */
