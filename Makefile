BLOCKSDS ?= /opt/wonderful/thirdparty/blocksds/core
NAME := castle-crashers-ds
GAME_TITLE := Castle Crashers DS
GAME_ICON := generated/game-icon.gif
GAME_SUBTITLE := Early build (unofficial)
GAME_AUTHOR := Unofficial fan demake
SOURCEDIRS := source generated
NITROFSDIR := nitrofiles
INCLUDEDIRS := source generated
LOCAL_RADIO_ONLY ?= 1
DEFERRED_RADIO ?= 1
ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_dswifi.elf
LIBS := -ldswifi9_noip -lnds9
LIBDIRS := $(BLOCKSDS)/libs/dswifi
ifeq ($(LEVEL_SELECT),1)
CFLAGS += -DLEVEL_SELECT_TEST
endif
ifeq ($(BOOT_DIAGNOSTIC),1)
CFLAGS += -DBOOT_DIAGNOSTIC
endif
ifeq ($(DEFERRED_RADIO),1)
ARM7ELF := build/arm7.elf
CFLAGS += -DDEFERRED_RADIO
endif
ifeq ($(LOCAL_RADIO_ONLY),1)
ARM7ELF := build/arm7-local.elf
endif
ifeq ($(BOOT_FTL_CORE),1)
ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_maxmod.elf
CFLAGS += -DBOOT_FTL_CORE
endif
include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

# Rebuild the cartridge banner when its title/subtitle changes.
$(ROM): Makefile $(ARM7ELF) $(GAME_ICON)

build/arm7-local.elf: $(wildcard arm7/source/*.c) Makefile.arm7
	$(MAKE) -f Makefile.arm7 LOCAL_RADIO_ONLY=1 BUILDDIR=build/arm7-local ELF=build/arm7-local.elf MAP=build/arm7-local.map

$(ROM): $(wildcard nitrofiles/audio/*) $(wildcard nitrofiles/chapters/*) $(wildcard nitrofiles/weapons/*)

