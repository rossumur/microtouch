###############################################################################
# Makefile for the project Microtouch
###############################################################################

## General Flags
PROJECT = Microtouch
TARGET = Microtouch.elf
CC = avr-gcc

# BOARD1
#MCU = atmega644p
#AVR_FREQ   = 12000000L

# BOARD2
MCU = atmega32u4
AVR_FREQ   = 16000000L


# Change if your programmer is different
AVRDUDE_PROGRAMMER = usbtiny
#AVRDUDE_PROGRAMMER = stk500v1
AVRDUDE_PORT = com5	   # programmer connected to serial device

# program name should not be changed...
PROGRAM    = Microtouch

AVRDUDE = avrdude
AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -p $(MCU)

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

override CFLAGS = -g -Wall -Os -mmcu=$(MCU) -DF_CPU=$(AVR_FREQ) $(DEFS) -ffunction-sections -gdwarf-2 -fdata-sections

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-gc-sections,-Map=Microtouch.map,--relax
##LDFLAGS += --section-start=.text=0x3C00

## Intel Hex file production flags
HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Define application folders here
## APPLICATIONS := apps/hardware
APPLICATIONS := apps/demos
## APPLICATIONS := apps/pacman
## APPLICATIONS := apps/3D

MODULES   := $(APPLICATIONS) apps/core platform hardware

## Wikipedia Demo
## Copy the data file onto a microSD card: tools/MicrotouchSim/MicrotouchSim/microSD/wiki.blb
## Uncomment the following line to build the Wikipedia test
## MODULES   := apps/wiki platform hardware

SRC_DIR   := $(addprefix src/,$(MODULES))
BUILD_DIR := $(addprefix build/,$(MODULES))

SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ       := $(patsubst src/%.cpp,build/%.o,$(SRC))
DEP		  := $(OBJ:%.o=%.d)
INCLUDES  := $(addprefix -I,$(SRC_DIR))

vpath %.cpp $(SRC_DIR)

.PHONY: all checkdirs clean

all: checkdirs $(TARGET) Microtouch.hex Microtouch.lss FIRMWARE.BIN size

-include $(DEP)

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf build/
	@rm -f *.hex
	@rm -f *.elf
	@rm -f *.lss
	@rm -f *.map
	@rm -f *.BIN
	
define make-goal
$1/%.o: %.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -c $$< -MD -o $$@
endef

$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) $^ -o $@

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

FIRMWARE.BIN: $(TARGET)
	avr-objcopy -O binary $< $@

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: $(TARGET)
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

program: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -B 1 -u -U flash:w:$(TARGET).hex