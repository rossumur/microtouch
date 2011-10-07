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

CFLAGS = -g -Wall -Os -mmcu=$(MCU) -DF_CPU=$(AVR_FREQ) $(DEFS) -ffunction-sections -gdwarf-2 -fdata-sections

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

#=========================================================================
## Define application folders here

## Default demos: Doomed, Lattice, Flip etc
MODULES  := apps/demos apps/core platform hardware

## Hardware related apps
ifeq ($(HARDWARE), 1)
	MODULES  := apps/core apps/hardware platform hardware
endif

## Pacman
ifeq ($(PACMAN), 1)
	MODULES  := apps/pacman platform hardware
	CFLAGS += -DNO_SHELL=1
endif

## 3D
ifeq ($(3D), 1)
	MODULES  := apps/3D platform hardware
	CFLAGS += -DNO_SHELL=1
endif

#=========================================================================
## Frotz (Zork) Demo
## Copy the game and page file (p.pge) onto a microSD card:
##	tools/MicrotouchSim/MicrotouchSim/microSD/game.z5
##	tools/MicrotouchSim/MicrotouchSim/microSD/p.pge

ifeq ($(ZORK), 1)
	MODULES := platform hardware apps/frotz apps/frotz/dumb-frotz-2.32r1
	CFLAGS += -DMIN_FLASH_SIZE=1 -DNO_SHELL=1
	CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
	CFLAGS += -fno-inline-small-functions -fno-split-wide-types -fno-tree-scev-cprop
	ifeq ($(CLEARTYPE), 1)
		CFLAGS += -DFROTZ_CLEARTYPE=1
	endif
endif

#=========================================================================
## Wikipedia Demo
## Copy the data file onto a microSD card: tools/MicrotouchSim/MicrotouchSim/microSD/wiki.blb

ifeq ($(WIKI), 1)
	MODULES := apps/wiki platform hardware
endif

#=========================================================================
## ebook Demo
## Copy the data file onto a microSD card: tools/MicrotouchSim/MicrotouchSim/microSD/books.epb

ifeq ($(EBOOK), 1)
	MODULES := apps/ebook platform hardware
	CFLAGS += -DMIN_FLASH_SIZE=1 -DNO_SHELL=1
endif

#=========================================================================


SRC_DIR   := $(addprefix src/,$(MODULES))
BUILD_DIR := $(addprefix build/,$(MODULES))

SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
SRC_C     := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ       := $(patsubst src/%.cpp,build/%.o,$(SRC))
OBJ       += $(patsubst src/%.c,build/%.o,$(SRC_C))

DEP		  := $(OBJ:%.o=%.d)
INCLUDES  := $(addprefix -I,$(SRC_DIR))

vpath %.cpp $(SRC_DIR)
vpath %.c $(SRC_DIR)

.PHONY: all checkdirs clean

all: checkdirs $(TARGET) Microtouch.hex Microtouch.lss FIRMWARE.BIN size

-include $(DEP)

checkdirs: $(BUILD_DIR)
	@echo BUILD_DIR: $(BUILD_DIR)

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
$1/%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $$< -MD -o $$@
endef

$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))

#==================================================

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) $^ -o $@ -lm

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
	
# Targets
# defaults to 'demo'

zorkinfo:
	@echo '\nBuilding Frotz (Zork) Demo\n'
	@echo 'Copy the game and page file (p.pge) onto a microSD card:'
	@echo '	tools/MicrotouchSim/MicrotouchSim/microSD/game.z5'
	@echo ' tools/MicrotouchSim/MicrotouchSim/microSD/p.pge\n\n'

zork: zorkinfo clean
	make ZORK=1
	@cp $(TARGET) prebuilt/microtouchzork/zork.hex
	
zorkcleartype: zorkinfo clean
	@echo '###### Using fancy cleartype font - need ISP to load firmware (>28k) ######\n\n'
	make ZORK=1 CLEARTYPE=1
	@cp $(TARGET) prebuilt/microtouchzork/zork.hex

wiki: clean
	make WIKI=1
	@cp $(TARGET) prebuilt/wikipedia.hex
	
ebook: clean
	make EBOOK=1
	@cp $(TARGET) prebuilt/ebook.hex
	
hardware: clean
	make HARDWARE=1
	@cp $(TARGET) prebuilt/hardware.hex

3d: clean
	make 3D=1
	@cp $(TARGET) prebuilt/3d.hex

pacman: clean
	make PACMAN=1
	@cp $(TARGET) prebuilt/pacman.hex

demo: clean
	make
	@cp $(TARGET) prebuilt/demos.hex
