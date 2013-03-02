#
# Makefile - msp430-ssbp
#

CC=msp430-gcc
CXX=msp430-g++
MCU=msp430g2553

CFLAGS=-mmcu=$(MCU) -O2 -g -Wall

APP=msp430-ssbp
TARGET=Debug

all: $(TARGET)/$(APP).elf

$(TARGET)/$(APP).elf: $(TARGET)/$(APP).o
	$(CC) $(CFLAGS) -o $(TARGET)/$(APP).elf $(TARGET)/$(APP).o
	msp430-objdump -DS $(TARGET)/$(APP).elf >$(TARGET)/$(APP).lst
	msp430-size $(TARGET)/$(APP).elf
	
$(TARGET)/$(APP).o:	$(APP).cpp $(APP).h 
	$(CC) $(CFLAGS) -c -o $(TARGET)/$(APP).o $(APP).cpp
	
install: $(TARGET/$(APP).o
	mspdebug -q --force-reset rf2500 "prog $(TARGET)/$(APP).elf"

clean:
	rm -f $(TARGET)/$(APP).o $(TARGET)/$(APP).elf $(TARGET)/$(APP).lst
	mkdir -p $(TARGET)/
