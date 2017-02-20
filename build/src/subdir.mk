# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Connect.cpp 

OBJS += \
./src/Connect.o 

CPP_DEPS += \
./src/Connect.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_UNIX_ -D_LINUX_ -I/opt/workswell/wic_sdk/include -I/opt/pleora/ebus_sdk/Ubuntu-14.04-x86_64/include -I/home/labuser/Boost/boost_1_62_0 -I/home/labuser/Boost/boost_1_62_0/boost/interprocess -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<" 
	@echo 'Finished building: $<'
	@echo ' '


