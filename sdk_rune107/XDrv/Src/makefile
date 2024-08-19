#=============================================================================
# Rune XDrv makefile for Linux.
#
# Revision history:
# 	* Created by Brandon Reinhart 
#	* Updated for Rune by Human Head
#=============================================================================

# Rune directory. (Required by makefile-header.)
RUNE_DIR	= ../..

# Include global definitions.
include $(RUNE_DIR)/makefile-header

#-----------------------------------------------------------------------------
# Directories.
#-----------------------------------------------------------------------------

SRC_DIRS	= $(XDRV_SRC)
BUILD_DIR	= $(XDRV_BUILD)

#-----------------------------------------------------------------------------
# Compiler/linker options.
#-----------------------------------------------------------------------------

CXX		= $(STD_CXX)
CXXFLAGS	= $(STD_CXXFLAGS) -Werror -fPIC -I$(XDRV_INC) \
			-I$(ENGINE_INC) -I$(CORE_INC) \
			-DGPackage=GPackageXDrv
LDFLAGS		= $(STD_LDFLAGS) -shared -Wl,-soname,$(XDRV_SONAME)
LIBS		= $(STD_LIBS) $(CORE) $(XLIBS) -lXxf86dga

#-----------------------------------------------------------------------------
# Files.
#-----------------------------------------------------------------------------

OBJS =			\
	XClient.o	\
	XDrv.o		\
	XViewport.o

OUT = $(XDRV)

#-----------------------------------------------------------------------------
# Rules.
#-----------------------------------------------------------------------------

# Use general rules.
include $(RUNE_DIR)/makefile-general

#-----------------------------------------------------------------------------
# The End.
#-----------------------------------------------------------------------------
