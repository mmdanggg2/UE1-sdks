#=============================================================================
# Rune XMesaGLDrv makefile for Linux.
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

SRC_DIRS	= $(XMESAGLDRV_SRC)
BUILD_DIR	= $(XMESAGLDRV_BUILD)

#-----------------------------------------------------------------------------
# Compiler/linker options.
#-----------------------------------------------------------------------------

CXX		= $(STD_CXX)
CXXFLAGS	= $(STD_CXXFLAGS) -Werror -fPIC -I$(RENDER_INC) \
			-I$(ENGINE_INC) -I$(CORE_INC) \
			-DGPackage=GPackageXMesaGLDrv
LDFLAGS		= $(STD_LDFLAGS) -shared -Wl,-soname,$(XMESAGLDRV_SONAME)
LIBS		= $(STD_LIBS) $(XLIBS) $(CORE)

#-----------------------------------------------------------------------------
# Files.
#-----------------------------------------------------------------------------

OBJS =			\
	XMesaGL.o	\
	XMesaGLDrv.o

OUT = $(XMESAGLDRV)

#-----------------------------------------------------------------------------
# Rules.
#-----------------------------------------------------------------------------

# Use general rules.
include $(RUNE_DIR)/makefile-general

#-----------------------------------------------------------------------------
# The End.
#-----------------------------------------------------------------------------
