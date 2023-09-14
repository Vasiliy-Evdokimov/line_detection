################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/barcode.cpp \
../src/camera.cpp \
../src/config.cpp \
../src/contours.cpp \
../src/horizontal.cpp \
../src/line_detection.cpp \
../src/shared_memory.cpp \
../src/udp.cpp 

CPP_DEPS += \
./src/barcode.d \
./src/camera.d \
./src/config.d \
./src/contours.d \
./src/horizontal.d \
./src/line_detection.d \
./src/shared_memory.d \
./src/udp.d 

OBJS += \
./src/barcode.o \
./src/camera.o \
./src/config.o \
./src/contours.o \
./src/horizontal.o \
./src/line_detection.o \
./src/shared_memory.o \
./src/udp.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++17 -I/usr/include/opencv4 -I/usr/local/include/ZXing -I/usr/local/include/drogon -I/usr/local/include/trantor -I/usr/include/openssl -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/barcode.d ./src/barcode.o ./src/camera.d ./src/camera.o ./src/config.d ./src/config.o ./src/contours.d ./src/contours.o ./src/horizontal.d ./src/horizontal.o ./src/line_detection.d ./src/line_detection.o ./src/shared_memory.d ./src/shared_memory.o ./src/udp.d ./src/udp.o

.PHONY: clean-src

