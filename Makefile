
prefix ?= /usr/local

# arm / x86_x64
PLATFORM_TYPE = arm
#
BOARD_TYPE = sophon_bm1684
# local / buildroot
TOOLCHAIN_TYPE = local
# 是否支持opencv yes / no
HAVE_OPENCV = no
# mqtt是否使用tls, 此处使能与goahead冲突
MQTT_USE_TLS = no

ifeq ($(PLATFORM_TYPE), arm)
CROSS_COMPILE = $(shell pwd)/../../buildroot-2023.02.4/output/$(BOARD_TYPE)/host/bin/aarch64-linux-gnu-
else
CROSS_COMPILE =
endif

INSTALL ?= /usr/bin/install

ifeq ($(TOOLCHAIN_TYPE), local)
AS          = $(CROSS_COMPILE)as
LD          = $(CROSS_COMPILE)ld
CC          = $(CROSS_COMPILE)gcc
CPP         = $(CROSS_COMPILE)g++
CXX         = $(CROSS_COMPILE)g++
AR          = $(CROSS_COMPILE)ar
NM          = $(CROSS_COMPILE)nm
STRIP       = $(CROSS_COMPILE)strip
OBJCOPY     = $(CROSS_COMPILE)objcopy
OBJDUMP     = $(CROSS_COMPILE)objdump

export AS LD CC CPP CXX AR NM
export STRIP OBJCOPY OBJDUMP
endif

#CFLAGS := -Wall -Werror -O2 -g
CFLAGS := -Wall -O2 -g -fPIC
CFLAGS += -I $(shell pwd)/include/comm
CFLAGS += -I $(shell pwd)/include/comm/blend
CFLAGS += -I $(shell pwd)/include/comm/mqttc

# so -- -L./lib -lzlog   (优先链接动态库，动态库不存在链接静态库)
# a  -- ./lib/libzlog.a
LDFLAGS := -lpthread -lm -lrt -levent -lexif
LDFLAGS += -Wl,-rpath=/usr/lib

ifeq ($(HAVE_OPENCV), yes)
CFLAGS += -D_OPENCV_
LDFLAGS += -lstdc++ -lopencv_core -lopencv_imgcodecs -lopencv_imgproc
endif

ifeq ($(MQTT_USE_TLS), no)
CFLAGS += -DMQTT_NETWORK_TYPE_NO_TLS
export MQTT_USE_TLS
endif

export CFLAGS LDFLAGS

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := libcomm.so

obj-y += src/

all : start_recursive_build $(TARGET)
	@echo $(TARGET) has been built!

start_recursive_build:
	make -C ./ -f $(TOPDIR)/Makefile.build

$(TARGET) : built-in.o
	$(CC) -shared -o $(TARGET) built-in.o $(LDFLAGS)


.PHONY: clean distclean install
clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)

install:
	$(INSTALL) -d $(DESTDIR)$(prefix)/lib
	$(INSTALL) -d $(DESTDIR)$(prefix)/include
	$(INSTALL) -m 644 $(TARGET) $(DESTDIR)$(prefix)/lib
	#cp $(TARGET) $(DESTDIR)$(prefix)/lib -rf
	cp include/comm $(DESTDIR)$(prefix)/include -rf
