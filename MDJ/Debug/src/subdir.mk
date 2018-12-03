################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MDJ.c \
../src/filesystemFIFA.c \
../src/interfaz.c 

OBJS += \
./src/MDJ.o \
./src/filesystemFIFA.o \
./src/interfaz.o 

C_DEPS += \
./src/MDJ.d \
./src/filesystemFIFA.d \
./src/interfaz.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2018-2c-smlc/sharedlib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


