################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.local

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_QUOTED += \
"../Sources/main.c" \
"../Sources/sa_mtb.c" \
"../Sources/structc.c" \

C_SRCS += \
../Sources/main.c \
../Sources/sa_mtb.c \
../Sources/structc.c \

OBJS += \
./Sources/main.o \
./Sources/sa_mtb.o \
./Sources/structc.o \

C_DEPS += \
./Sources/main.d \
./Sources/sa_mtb.d \
./Sources/structc.d \

OBJS_QUOTED += \
"./Sources/main.o" \
"./Sources/sa_mtb.o" \
"./Sources/structc.o" \

C_DEPS_QUOTED += \
"./Sources/main.d" \
"./Sources/sa_mtb.d" \
"./Sources/structc.d" \

OBJS_OS_FORMAT += \
./Sources/main.o \
./Sources/sa_mtb.o \
./Sources/structc.o \


# Each subdirectory must supply rules for building sources it contributes
Sources/main.o: ../Sources/main.c
	@echo 'Building file: $<'
	@echo 'Executing target #1 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/main.args" -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/main.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/sa_mtb.o: ../Sources/sa_mtb.c
	@echo 'Building file: $<'
	@echo 'Executing target #2 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/sa_mtb.args" -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/sa_mtb.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/structc.o: ../Sources/structc.c
	@echo 'Building file: $<'
	@echo 'Executing target #3 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/structc.args" -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/structc.o"
	@echo 'Finished building: $<'
	@echo ' '


