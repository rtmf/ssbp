#
# Makefile - msp430-ssbp
#

CC=msp430-gcc
CXX=msp430-g++
MCU=msp430g2553

CFLAGS=-mmcu=$(MCU) -O2 -g -Wall

APP=msp430-ssbp
TARGET=Debug
SOURCES=$(APP).cpp
HEADERS=
FILES=$(SOURCES) $(HEADERS)

all: $(TARGET)/$(APP)

jpop: $(TARGET)/jpop

$(TARGET)/jpop:		$(FILES)
	mkdir -p $(TARGET)/
	$(CC) $(CFLAGS) -DJPOP -o $(TARGET)/jpop $(SOURCES)

$(TARGET)/$(APP):	$(FILES)
	mkdir -p $(TARGET)/
	$(CC) $(CFLAGS) -o $(TARGET)/$(APP) $(SOURCES)
	
install:		$(TARGET)/$(APP) $(FILES)
	mspdebug -q --force-reset rf2500 "prog $(TARGET)/$(APP)"

install-jpop:		$(TARGET)/jpop $(FILES)
	mspdebug -q --force-reset rf2500 "prog $(TARGET)/jpop"

clean:
	rm -f $(TARGET)/$(APP) 
	rm -f $(TARGET)/jpop
	mkdir -p $(TARGET)/
