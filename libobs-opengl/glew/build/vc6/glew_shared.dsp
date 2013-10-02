# Microsoft Developer Studio Project File - Name="glew_shared" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=glew_shared - Win32 Debug MX
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "glew_shared.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "glew_shared.mak" CFG="glew_shared - Win32 Debug MX"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "glew_shared - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "glew_shared - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "glew_shared - Win32 Debug MX" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "glew_shared - Win32 Release MX" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "glew_shared - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../lib"
# PROP Intermediate_Dir "shared/release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLEW_EXPORTS" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /I "../../include" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "VC_EXTRALEAN" /D "GLEW_BUILD" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 opengl32.lib /nologo /dll /pdb:none /machine:I386 /out:"../../bin/glew32.dll" /ignore:4089
# ADD LINK32 /base:0x62AA0000

!ELSEIF  "$(CFG)" == "glew_shared - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../lib"
# PROP Intermediate_Dir "shared/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLEW_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Zi /Od /I "../../include" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "GLEW_BUILD" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"../../bin/glew32d.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# ADD LINK32 /base:0x62AA0000

!ELSEIF  "$(CFG)" == "glew_shared - Win32 Debug MX"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "glew_shared___Win32_Debug_MX"
# PROP BASE Intermediate_Dir "glew_shared___Win32_Debug_MX"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../lib"
# PROP Intermediate_Dir "shared/debug-mx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Zi /Od /I "../../include" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "GLEW_BUILD" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Zi /Od /I "../../include" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "GLEW_MX" /D "GLEW_BUILD" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "GLEW_MX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 opengl32.lib /nologo /dll /incremental:no /pdb:"../../lib/glew32d.pdb" /debug /machine:I386 /out:"../../lib/glew32d.dll" /implib:"../../lib/glew32d.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 opengl32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"../../bin/glew32mxd.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# ADD LINK32 /base:0x62AA0000

!ELSEIF  "$(CFG)" == "glew_shared - Win32 Release MX"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "glew_shared___Win32_Release_MX"
# PROP BASE Intermediate_Dir "glew_shared___Win32_Release_MX"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../lib"
# PROP Intermediate_Dir "shared/release-mx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "../../include" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "VC_EXTRALEAN" /D "GLEW_BUILD" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /I "../../include" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "VC_EXTRALEAN" /D "GLEW_MX" /D "GLEW_BUILD" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "GLEW_MX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 opengl32.lib /nologo /dll /pdb:none /machine:I386 /out:"../../lib/glew32.dll" /implib:"../../lib/glew32.lib" /ignore:4089
# ADD LINK32 opengl32.lib /nologo /dll /pdb:none /machine:I386 /out:"../../bin/glew32mx.dll" /ignore:4089
# ADD LINK32 /base:0x62AA0000

!ENDIF 

# Begin Target

# Name "glew_shared - Win32 Release"
# Name "glew_shared - Win32 Debug"
# Name "glew_shared - Win32 Debug MX"
# Name "glew_shared - Win32 Release MX"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\glew.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\GL\glew.h
# End Source File
# Begin Source File

SOURCE=..\..\include\GL\wglew.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\glew.rc
# End Source File
# End Group
# End Target
# End Project
