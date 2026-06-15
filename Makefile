PROJECT := plipbox13

AMIGA_PREFIX ?= /opt/amiga
NETINCLUDE_DIR ?= /opt/amiga-netinclude
SANA_INCLUDE_DIR ?= /opt/megaburken.net/~patrik/NDK3.2R4/SANA+RoadshowTCP-IP/include
CROSS := $(AMIGA_PREFIX)/bin/m68k-amigaos-
CC := $(CROSS)gcc
AS := $(AMIGA_PREFIX)/bin/vasmm68k_mot

PLIPBOX_SRC_DIR := vendor/plipbox-0.5/amiga/src
SRC_DIR := src/plipbox13
BUILD_DIR := build

CPPFLAGS := \
	-I$(SRC_DIR) \
	-I$(PLIPBOX_SRC_DIR) \
	-I$(PLIPBOX_SRC_DIR)/plipbox \
	-I$(PLIPBOX_SRC_DIR)/utility \
	-Iinclude \
	-I$(AMIGA_PREFIX)/m68k-amigaos/ndk-include \
	-I$(SANA_INCLUDE_DIR) \
	-I$(NETINCLUDE_DIR)/include \
	-include utility/tagitem.h \
	-include utility/hooks.h \
	-DDEVICE_VERSION=0 \
	-DDEVICE_REVISION=5 \
	-DPLIPBOX_GCC_DIRECT=1 \
	-DPLIPBOX_OS13=1

CFLAGS := -O2 -Wall -Wextra -Wno-unused-function -mcrt=nix13 -fno-builtin
ASFLAGS := -quiet -Faout -m68000 \
	-I$(AMIGA_PREFIX)/m68k-amigaos/ndk-include \
	-I$(PLIPBOX_SRC_DIR) \
	-I$(PLIPBOX_SRC_DIR)/plipbox

PLIPBOX13_HW_STAGE ?= 3
PLIPBOX13_SKIP_CIA_ICR ?= 1
PLIPBOX13_ATTACH_STAGE ?= 6
PLIPBOX13_SKIP_MISC_ALLOC ?= 1
PLIPBOX13_SKIP_MAGIC ?= 1
PLIPBOX13_DIRECT_HWSEND ?= 0
PLIPBOX13_HANDSHAKE_DIAG ?= 0
PLIPBOX13_C_SEND ?= 1

CPPFLAGS += \
	-DPLIPBOX13_HW_STAGE=$(PLIPBOX13_HW_STAGE) \
	-DPLIPBOX13_SKIP_CIA_ICR=$(PLIPBOX13_SKIP_CIA_ICR) \
	-DPLIPBOX13_ATTACH_STAGE=$(PLIPBOX13_ATTACH_STAGE) \
	-DPLIPBOX13_SKIP_MISC_ALLOC=$(PLIPBOX13_SKIP_MISC_ALLOC) \
	-DPLIPBOX13_SKIP_MAGIC=$(PLIPBOX13_SKIP_MAGIC) \
	-DPLIPBOX13_DIRECT_HWSEND=$(PLIPBOX13_DIRECT_HWSEND) \
	-DPLIPBOX13_HANDSHAKE_DIAG=$(PLIPBOX13_HANDSHAKE_DIAG) \
	-DPLIPBOX13_C_SEND=$(PLIPBOX13_C_SEND)

C_OBJS := \
	$(BUILD_DIR)/device.o \
	$(BUILD_DIR)/hwshim.o \
	$(BUILD_DIR)/hw.o

AS_OBJS := \
	$(BUILD_DIR)/romtag.o \
	$(BUILD_DIR)/wrappers.o \
	$(BUILD_DIR)/hwpar.o

OBJS := $(C_OBJS) $(AS_OBJS)

.PHONY: all clean plipbox-device

all: plipbox-device

plipbox-device: $(BUILD_DIR)/plipbox.device

$(BUILD_DIR)/plipbox.device: $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -nostartfiles -nostdlib -o $@ $(OBJS) \
		-L$(AMIGA_PREFIX)/m68k-amigaos/lib -lamiga \
		-L$(AMIGA_PREFIX)/lib/gcc/m68k-amigaos/6.5.0b -lgcc

$(BUILD_DIR)/device.o: $(SRC_DIR)/device.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/hwshim.o: $(SRC_DIR)/hwshim.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/hw.o: $(PLIPBOX_SRC_DIR)/hw.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/romtag.o: $(SRC_DIR)/romtag.asm
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/wrappers.o: $(SRC_DIR)/wrappers.asm
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/hwpar.o: $(PLIPBOX_SRC_DIR)/hwpar.asm
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
