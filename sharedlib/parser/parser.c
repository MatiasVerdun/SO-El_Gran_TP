#include "parser.h"

#define RETURN_ERROR t_parser_operacion ERROR={ .valido = false }; return ERROR
#define ABRIR_KEYWORD "abrir"
#define CONCENTRAR_KEYWORD "concentrar"
#define ASIGNAR_KEYWORD "asignar"
#define WAIT_KEYWORD "wait"
#define SIGNAL_KEYWORD "signal"
#define FLUSH_KEYWORD "flush"
#define CLOSE_KEYWORD "close"
#define CREAR_KEYWORD "crear"
#define BORRAR_KEYWORD "borrar"


t_parser_operacion parse(char* line){
	if(line == NULL || string_equals_ignore_case(line, "")){
		fprintf(stderr, "No pude interpretar una linea vacia\n");
		RETURN_ERROR;
	}

	t_parser_operacion ret = {
		.valido = true
	};

	char* auxLine = string_duplicate(line);
	string_trim(&auxLine);
	char** split = string_n_split(auxLine, 4, " ");

	char* keyword = split[0];

	ret._raw = split;

	if(string_equals_ignore_case(keyword, ABRIR_KEYWORD)){
		ret.keyword = ABRIR;
		ret.argumentos.ABRIR.param1 = split[1];
	} else if(string_equals_ignore_case(keyword, CONCENTRAR_KEYWORD)){
		ret.keyword = CONCENTRAR;
	} else if(string_equals_ignore_case(keyword, ASIGNAR_KEYWORD)){
		ret.keyword = ASIGNAR;
		ret.argumentos.ASIGNAR.param1 = split[1];
		ret.argumentos.ASIGNAR.param2 = atoi(split[2]);
		ret.argumentos.ASIGNAR.param3 = split[3];
	} else if(string_equals_ignore_case(keyword, WAIT_KEYWORD)){
		ret.keyword = WAIT;
		ret.argumentos.WAIT.param1 = split[1];
	} else if(string_equals_ignore_case(keyword, SIGNAL_KEYWORD)){
		ret.keyword = SIGNAL;
		ret.argumentos.SIGNAL.param1 = split[1];
	} else if(string_equals_ignore_case(keyword, FLUSH_KEYWORD)){
		ret.keyword = FLUSH;
		ret.argumentos.FLUSH.param1 = split[1];
	} else if(string_equals_ignore_case(keyword, CLOSE_KEYWORD)){
		ret.keyword = CLOSE;
		ret.argumentos.CLOSE.param1 = split[1];
	} else if(string_equals_ignore_case(keyword, CREAR_KEYWORD)){
		ret.keyword = CREAR;
		ret.argumentos.CREAR.param1 = split[1];
		ret.argumentos.CREAR.param2 = atoi(split[2]);
	} else if(string_equals_ignore_case(keyword, BORRAR_KEYWORD)){
		ret.keyword = BORRAR;
		ret.argumentos.BORRAR.param1 = split[1];
	} else {
		fprintf(stderr, "No se encontro el keyword <%s>\n", keyword);
		RETURN_ERROR;
	}

	free(auxLine);
	return ret;
}
