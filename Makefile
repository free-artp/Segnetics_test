# export PATH=/opt/linaro/arm-linux-gnueabihf/bin:$PATH
# /opt/linaro/sysroot

ARCH=armv7-a
CROSS_COMPILE=arm-linux-gnueabihf

TARGET=test2
SOURCES = main.c shmem.c vars.c config.c

DEFINES=
INCLUDES = -I../libs/include -Isrc

ODIR = obj
SDIR = src

#OBJECTS = $(SOURCES:%.c=%.o)
#OBJS = $(patsubst %,$(ODIR)/%,$(OBJECTS))
OBJECTS = $(patsubst %,$(ODIR)/%,$(SOURCES:%.c=%.o))
_TARGET = $(patsubst %,$(ODIR)/%,$(TARGET))

TOOLCHAIN := $(shell cd $(dir $(shell which $(CROSS_COMPILE)-gcc))/.. && pwd -P)
SYSROOT := $(TOOLCHAIN)/../sysroot

CC=$(CROSS_COMPILE)-gcc
NM=$(CROSS_COMPILE)-nm
LD=$(CROSS_COMPILE)-ld
CXX=$(CROSS_COMPILE)-g++
RANLIB=$(CROSS_COMPILE)-ranlib
AR=$(CROSS_COMPILE)-ar
CPP=$(CROSS_COMPILE)-cpp
OBJCOPY=$(CROSS_COMPILE)-objcopy

CCFLAGS =  -g -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -O0 -Wall -W -fPIE $(INCLUDES) $(DEFINES)
LDFLAGS = -L../libs/lib -lsqlite3 -lpthread

all : $(_TARGET)


install: $(_TARGET)
	@scp $(_TARGET) root@192.168.1.213:$(TARGET)

$(_TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CCFLAGS) 

clean :
	@rm -f $(_TARGET) $(OBJECTS)
	@rm -f core
.PHONY : clean info


info:
	@echo $(TOOLCHAIN)
	@echo $(CROSS_COMPILE)
	@echo $(SYSROOT)
	@echo $(OBJECTS)
	@echo $(TARGET)
	@echo $(_TARGET)