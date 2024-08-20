# Microsoft Developer Studio Project File - Name="OpenGLDrv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OpenGLDrv - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "OpenGLDrv.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "OpenGLDrv.mak" CFG="OpenGLDrv - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "OpenGLDrv - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "OpenGLDrv - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/UT420/OpenGLDrv/Src", HFMAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OpenGLDrv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Lib"
# PROP Intermediate_Dir "Lib"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OpenGLDRV_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /Zp4 /MD /W4 /vd0 /GX /O2 /Ob2 /I "..\Core\Inc" /I "..\Engine\Inc" /I "..\UnExt\Inc" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "UNICODE" /D "_UNICODE" /D ThisPackage=OpenGLDrv /FR /FD /c
# SUBTRACT CPP /WX /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\Core\Lib\Core.lib ..\Engine\Lib\Engine.lib user32.lib gdi32.lib opengl32.lib /nologo /dll /machine:I386 /out:"..\System\OpenGLDrv.dll"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "OpenGLDrv - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Lib"
# PROP Intermediate_Dir "Lib"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OpenGLDRV_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Zp4 /MDd /W4 /WX /Gm /vd0 /GX /ZI /Od /I "..\..\Core\Inc" /I "..\..\Engine\Inc" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "UNICODE" /D "_UNICODE" /D ThisPackage=OpenGLDrv /Yu"OpenGLDrv.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\Core\Lib\Core.lib ..\Engine\Lib\Engine.lib user32.lib gdi32.lib wingdi.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"..\System\OpenGLDrv.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "OpenGLDrv - Win32 Release"
# Name "OpenGLDrv - Win32 Debug"
# Begin Group "Src"

# PROP Default_Filter "*.cpp;*.h"
# Begin Source File

SOURCE=.\Src\glcorearb.h
# End Source File
# Begin Source File

SOURCE=.\Src\glext.h
# End Source File
# Begin Source File

SOURCE=.\Src\glxext.h
# End Source File
# Begin Source File

SOURCE=.\Src\OpenGL.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\OpenGLDrv.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\OpenGLDrv.h
# End Source File
# Begin Source File

SOURCE=.\Src\OpenGLFuncs.h
# End Source File
# Begin Source File

SOURCE=.\Src\wglext.h
# End Source File
# Begin Source File

SOURCE=.\Src\WGLFuncs.h
# End Source File
# End Group
# Begin Group "Int"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\System\OpenGLDrv.int
# End Source File
# End Group
# End Target
# End Project
