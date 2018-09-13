#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "commons/string.h"

#ifndef CONSOLE_MYCONSOLE_H_
#define CONSOLE_MYCONSOLE_H_
#define MICROSEC 1000000
#define BLACK     "\x1b[30m"
#define RED     "\x1b[31m"
#define GREEN   "\e[0;32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\e[0;94m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE    "\x1b[37m"
#define BOLDBLACK   "\033[1m\033[30m"
#define BOLDRED     "\033[1m\033[31m"
#define BOLDGREEN   "\033[1m\033[32m"
#define BOLDYELLOW  "\033[1m\033[33m"
#define BOLDBLUE    "\e[1;94m"
#define BOLDMAGENTA "\033[1m\033[35m"
#define BOLDCYAN    "\033[1m\033[36m"
#define BOLDWHITE   "\033[1m\033[37m"
#define COLOR_RESET   "\x1b[0m"

void myPuts(const char* format, ...);
void displayC(char character,int times);
void displayBoxTitle(int longitud,char* texto);
void displayBoxBody(int longitud,char* texto);
void displayBoxClose(int longitud);
void loading(int timesec);

#endif /* CONSOLE_MYCONSOLE_H_ */
