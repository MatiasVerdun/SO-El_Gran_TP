
#ifndef CONSOLE_MYCONSOLE_H_
#define CONSOLE_MYCONSOLE_H_

#define BLACK     "\x1b[30m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE    "\x1b[37m"
#define COLOR_RESET   "\x1b[0m"

void myPuts(const char* format, ...);
void displayC(char character,int times);
void displayBoxTitle(int longitud,char* texto);
void displayBoxBody(int longitud,char* texto);
void displayBoxClose(int longitud);

#endif /* CONSOLE_MYCONSOLE_H_ */
