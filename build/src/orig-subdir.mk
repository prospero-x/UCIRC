# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/WIC_SDK_Sample.cpp 

OBJS += \
./src/WIC_SDK_Sample.o 

CPP_DEPS += \
./src/WIC_SDK_Sample.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_UNIX_ -D_LINUX_ -I/opt/workswell/wic_sdk/include -I/opt/pleora/ebus_sdk/Ubuntu-14.04-x86_64/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


