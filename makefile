OBJ := CPU.o DAM.o FM9.o MDJ.o S-AFA.o

# CURRENT_DIR := $(shell pwd)
DIR := ${CURDIR}

H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')

HEADERS := -I"/usr/include" -I"$(DIR)/sharedlib" -I"$(DIR)/CPU" -I"$(DIR)/DAM" -I"$(DIR)/FM9" -I"$(DIR)/MDJ" -I"$(DIR)/S-AFA"         
LIBPATH := -L"$(DIR)/sharedlib"
LIBS := -lcommons -lpthread -lreadline -lsharedlib

CC := gcc -w -g
CFLAGS := -std=c11 $(HEADERS)

# Cualquier cosa aca hay un ejemplo http://www.network-theory.co.uk/docs/gccintro/gccintro_22.html

#$(BIN): $(OBJ)
#$(CC) $(OBJ) -o $(BIN) $(CFLAGS) $(LDFLAGS) $(LDLIBS)
# letra i mayuscula -I<Directory> sirve para los header files
# -L<Directory> directorio de las bibliotecas
# letra ele minuscula -l<nombredelarchivo> sin el .c al final



# -------------------------------------------------------------------------------------------------------------------------------------



# All

all: libobjs CPU DAM FM9 MDJ S-AFA



# -------------------------------------------------------------------------------------------------------------------------------------

# CPU

CPU: CPU.o
	$(CC) $(LIBPATH) CPU.o -o $(DIR)/CPU/src/CPU $(LIBS)


CPU.o:
	$(CC) $(CFLAGS) -c $(DIR)/CPU/src/CPU.c -o CPU.o




# DAM

DAM: DAM.o
	$(CC) $(LIBPATH) DAM.o -o $(DIR)/DAM/src/DAM $(LIBS)


DAM.o:
	$(CC) $(CFLAGS) -c $(DIR)/DAM/src/DAM.c -o DAM.o




# FM9

FM9: FM9.o
	$(CC) $(LIBPATH) FM9.o -o  $(DIR)/FM9/src/FM9 $(LIBS)


FM9.o:
	$(CC) -c $(CFLAGS) $(DIR)/FM9/src/FM9.c -o FM9.o




# MDJ

MDJ: MDJ.o filesystemFIFA.o interfaz.o
	$(CC) $(LIBPATH) MDJ.o filesystemFIFA.o interfaz.o -o $(DIR)/MDJ/src/MDJ $(LIBS)

filesystemFIFA.o:
	$(CC) $(CFLAGS) -c $(CFLAGS) $(DIR)/filesystemFIFA/src/filesystemFIFA.c -o filesystemFIFA.o

interfaz.o:
	$(CC) $(CFLAGS) -c $(CFLAGS) $(DIR)/interfaz/src/interfaz.c -o interfaz.o

MDJ.o:
	$(CC) $(CFLAGS) -c $(CFLAGS) $(DIR)/MDJ/src/MDJ.c -o MDJ.o




# S-AFA


S-AFA: S-AFA.o
	$(CC) $(LIBPATH) S-AFA.o -o $(DIR)/S-AFA/src/S-AFA $(LIBS)


S-AFA.o:
	$(CC) $(CFLAGS) -c $(CFLAGS) $(DIR)/S-AFA/src/S-AFA.c -o S-AFA.o



# -------------------------------------------------------------------------------------------------------------------------------------



# Libraries


libclean:
	$(MAKE) uninstall -C $(DIR)/commons_lib/so-commons-library
	$(MAKE) clean -C $(DIR)/commons_lib/so-commons-library


lib:
	$(MAKE) -C $(DIR)/commons_lib/so-commons-library
	$(MAKE) install -C $(DIR)/commons_lib/so-commons-library

#-------------------------------------------------------------------------------------------------------------------------------------

archivos.o:
	$(CC) -c $(CFLAGS) $(DIR)/sharedlib/archivos/archivos.c -o $(DIR)/sharedlib/Release/archivos/archivos.o

conexiones.o:
	$(CC) -c $(CFLAGS) $(DIR)/sharedlib/conexiones/mySockets.c -o $(DIR)/sharedlib/Release/conexiones/mySockets.o

console.o:
	$(CC) -c $(CFLAGS) $(DIR)/sharedlib/console/myConsole.c -o $(DIR)/sharedlib/Release/console/myConsole.o

dtbSerializacion.o:
	$(CC) -c $(CFLAGS) $(DIR)/sharedlib/dtbSerializacion/dtbSerializacion.c -o $(DIR)/sharedlib/Release/dtbSerializacion/dtbSerializacion.o

parser.o:
	$(CC) -c $(CFLAGS) $(DIR)/sharedlib/parser/parser.c -o $(DIR)/sharedlib/Release/parser/parser.o

libobjs: archivos.o conexiones.o console.o dtbSerializacion.o parser.o
		$(MAKE) -C $(DIR)/sharedlib/Release

