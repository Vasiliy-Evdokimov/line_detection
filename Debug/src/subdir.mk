################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/barcode.cpp \
../src/calibration.cpp \
../src/camera.cpp \
../src/config.cpp \
../src/config_path.cpp \
../src/contours.cpp \
../src/crc.cpp \
../src/horizontal.cpp \
../src/line_detection.cpp \
../src/log.cpp \
../src/shared_memory.cpp \
../src/templates.cpp \
../src/udp.cpp 

CPP_DEPS += \
./src/barcode.d \
./src/calibration.d \
./src/camera.d \
./src/config.d \
./src/config_path.d \
./src/contours.d \
./src/crc.d \
./src/horizontal.d \
./src/line_detection.d \
./src/log.d \
./src/shared_memory.d \
./src/templates.d \
./src/udp.d 

OBJS += \
./src/barcode.o \
./src/calibration.o \
./src/camera.o \
./src/config.o \
./src/config_path.o \
./src/contours.o \
./src/crc.o \
./src/horizontal.o \
./src/line_detection.o \
./src/log.o \
./src/shared_memory.o \
./src/templates.o \
./src/udp.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++17 -I/usr/include/opencv4 -I/usr/local/include/ZXing -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/barcode.d ./src/barcode.o ./src/calibration.d ./src/calibration.o ./src/camera.d ./src/camera.o ./src/config.d ./src/config.o ./src/config_path.d ./src/config_path.o ./src/contours.d ./src/contours.o ./src/crc.d ./src/crc.o ./src/horizontal.d ./src/horizontal.o ./src/line_detection.d ./src/line_detection.o ./src/log.d ./src/log.o ./src/shared_memory.d ./src/shared_memory.o ./src/templates.d ./src/templates.o ./src/udp.d ./src/udp.o

.PHONY: clean-src

