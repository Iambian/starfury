# ----------------------------
# Set NAME to the program name
# Set DEBUGMODE to "DEBUG" to use debug functions
# Set ICON to the png icon file name
# Set DESCRIPTION to display within a compatible shell
# Set COMPRESSED to "YES" to create a compressed program
# ** Add all shared library names to L **
# ----------------------------

NAME        ?= STARFURY
DEBUGMODE   ?= DEBUG
COMPRESSED  ?= NO
QUICK_BATTLE ?=
ICON        ?= iconc.png
DESCRIPTION ?= "Intergalactic Space Taxi Starfury"
ARCHIVED    ?= NO

L ?= 
PCSHIM_EXTRA_SOURCE_DIRS ?=

CONFIG_TOOL ?= cedev-config
ifeq ($(PC),1)
CONFIG_TOOL = ..\..\bin\pcshim-config.bat
PCSHIM_EXTRA_SOURCE_DIRS = host
endif

# ----------------------------
# Specify source and output locations
# ----------------------------

SRCDIR ?= src
OBJDIR ?= obj
BINDIR ?= bin
GFXDIR ?= src/gfx

# ----------------------------
# Use OS helper functions (Advanced)
# ----------------------------

USE_FLASH_FUNCTIONS ?= YES

ifeq ($(strip $(QUICK_BATTLE)),1)
CFLAGS += -DSTARFURY_QUICK_BATTLE=1
endif

include $(shell $(CONFIG_TOOL) --makefile)
