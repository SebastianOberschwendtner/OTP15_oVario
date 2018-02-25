################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/oVario_Framework.c \
../src/syscalls.c \
../src/system_stm32f4xx.c 

OBJS += \
./src/main.o \
./src/oVario_Framework.o \
./src/syscalls.o \
./src/system_stm32f4xx.o 

C_DEPS += \
./src/main.d \
./src/oVario_Framework.d \
./src/syscalls.d \
./src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32F405RGTx -DSTM32F4 -DSTM32 -DDEBUG -DUSE_STDPERIPH_DRIVER -DSTM32F40_41xxx -I"/Volumes/Macintosh HD/Documents/Eclipse/OTP15_oVario/inc" -I"/Volumes/Macintosh HD/Documents/Eclipse/OTP15_oVario/CMSIS/core" -I"/Volumes/Macintosh HD/Documents/Eclipse/OTP15_oVario/CMSIS/device" -I"/Volumes/Macintosh HD/Documents/Eclipse/OTP15_oVario/StdPeriph_Driver/inc" -I"/Volumes/Macintosh HD/Documents/Eclipse/OTP15_oVario/driver" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


