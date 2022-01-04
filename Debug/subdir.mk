################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../apiServer.c \
../client.c \
../coda.c \
../comunicazioneClientServer.c \
../gestioneFile.c \
../server.c \
../utility.c \
../worker.c 

O_SRCS += \
../apiServer.o \
../client.o \
../coda.o \
../comunicazioneClientServer.o \
../gestioneFile.o \
../server.o \
../utility.o \
../worker.o 

OBJS += \
./apiServer.o \
./client.o \
./coda.o \
./comunicazioneClientServer.o \
./gestioneFile.o \
./server.o \
./utility.o \
./worker.o 

C_DEPS += \
./apiServer.d \
./client.d \
./coda.d \
./comunicazioneClientServer.d \
./gestioneFile.d \
./server.d \
./utility.d \
./worker.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


