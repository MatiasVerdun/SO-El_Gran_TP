#include "myConsole.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "commons/string.h"

void displayC(char character,int times){
	for (int i=0;i<times;i++){
		myPuts("%c",character);
	}
}

void displayBoxTitle(int longitud,char* texto){
	myPuts(BLUE "+");
	displayC('-',longitud);
	myPuts("+\n");
	myPuts("|"COLOR_RESET);
	displayC(' ',(longitud/2)-1-(strlen(texto)/2));
	myPuts(MAGENTA "%s" COLOR_RESET ,texto);
	displayC(' ',longitud-((longitud/2)-1-(strlen(texto)/2)+strlen(texto)));
	myPuts(BLUE"|\n");
	myPuts("+");
	displayC('-',longitud);
	myPuts("+" COLOR_RESET "\n");
}

void displayBoxBody(int longitud,char* texto){
	myPuts(BLUE "|"COLOR_RESET);
	displayC(' ',1);
	myPuts(CYAN "%s" COLOR_RESET ,texto);
	displayC(' ',longitud-1-strlen(texto));
	myPuts(BLUE"|" COLOR_RESET "\n");
}

void displayBoxClose(int longitud){
	myPuts(BLUE"+");
	displayC('-',longitud);
	myPuts("+" COLOR_RESET "\n");
}

void myPuts(const char* format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char* nuevo = string_from_vformat(format, arguments);
	va_end(arguments);
	fputs(nuevo, stdout);
	free(nuevo);
}
