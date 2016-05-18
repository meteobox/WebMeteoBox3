################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=
LINKER_SCRIPT_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS :=  \
../Source


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../Source/dhcp_client.c \
../Source/DHT.c \
../Source/dnslkup.c \
../Source/i2csoft.c \
../Source/main.c \
../Source/enc28j60.c \
../Source/ip_arp_udp_tcp.c \
../Source/OWIcrc.c \
../Source/OWIHighLevelFunctions.c \
../Source/OWISWBitFunctions.c \
../Source/protocol.c \
../Source/task_config.c \
../Source/term.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
Source/dhcp_client.o \
Source/DHT.o \
Source/dnslkup.o \
Source/i2csoft.o \
Source/main.o \
Source/enc28j60.o \
Source/ip_arp_udp_tcp.o \
Source/OWIcrc.o \
Source/OWIHighLevelFunctions.o \
Source/OWISWBitFunctions.o \
Source/protocol.o \
Source/task_config.o \
Source/term.o

OBJS_AS_ARGS +=  \
Source/dhcp_client.o \
Source/DHT.o \
Source/dnslkup.o \
Source/i2csoft.o \
Source/main.o \
Source/enc28j60.o \
Source/ip_arp_udp_tcp.o \
Source/OWIcrc.o \
Source/OWIHighLevelFunctions.o \
Source/OWISWBitFunctions.o \
Source/protocol.o \
Source/task_config.o \
Source/term.o

C_DEPS +=  \
Source/dhcp_client.d \
Source/DHT.d \
Source/dnslkup.d \
Source/i2csoft.d \
Source/main.d \
Source/enc28j60.d \
Source/ip_arp_udp_tcp.d \
Source/OWIcrc.d \
Source/OWIHighLevelFunctions.d \
Source/OWISWBitFunctions.d \
Source/protocol.d \
Source/task_config.d \
Source/term.d

C_DEPS_AS_ARGS +=  \
Source/dhcp_client.d \
Source/DHT.d \
Source/dnslkup.d \
Source/i2csoft.d \
Source/main.d \
Source/enc28j60.d \
Source/ip_arp_udp_tcp.d \
Source/OWIcrc.d \
Source/OWIHighLevelFunctions.d \
Source/OWISWBitFunctions.d \
Source/protocol.d \
Source/task_config.d \
Source/term.d

OUTPUT_FILE_PATH +=WebMeteoBox3.elf

OUTPUT_FILE_PATH_AS_ARGS +=WebMeteoBox3.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+= 

LINKER_SCRIPT_DEP+= 


# AVR32/GNU C Compiler



























Source/%.o: ../Source/%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : 4.9.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe$(QUOTE)  -x c -funsigned-char -funsigned-bitfields -DDEBUG -DF_CPU=8000000 -D__AVR_LIBC_DEPRECATED_ENABLE__  -I"C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATmega_DFP\1.0.98\include"  -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=atmega128 -B "C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATmega_DFP\1.0.98\gcc\dev\atmega128" -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU Linker : 4.9.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="WebMeteoBox3.map" -Wl,--start-group -Wl,-lm -Wl,-lprintf_flt  -Wl,--end-group -Wl,-L"C:\MeteoBox3\WebMeteoBox3\WebMeteoBox3"  -Wl,--gc-sections -mrelax -mmcu=atmega128 -B "C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATmega_DFP\1.0.98\gcc\dev\atmega128" -Wl,-u,vfprintf -lprintf_flt -lm  
	@echo Finished building target: $@
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "WebMeteoBox3.elf" "WebMeteoBox3.hex"
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-size.exe" "WebMeteoBox3.elf"
	
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "WebMeteoBox3.elf" "WebMeteoBox3.a" "WebMeteoBox3.hex" "WebMeteoBox3.lss" "WebMeteoBox3.eep" "WebMeteoBox3.map" "WebMeteoBox3.srec" "WebMeteoBox3.usersignatures"
	