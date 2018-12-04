################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../conexiones/mySockets.c 

OBJS += \
./conexiones/mySockets.o 

C_DEPS += \
./conexiones/mySockets.d 


# Each subdirectory must supply rules for building sources it contributes
conexiones/%.o: ../conexiones/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


