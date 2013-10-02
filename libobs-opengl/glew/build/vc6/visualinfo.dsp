# Microsoft Developer Studio Project File - Name="visualinfo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=visualinfo - Win32 Debug MX
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "visualinfo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "visualinfo.mak" CFG="visualinfo - Win32 Debug MX"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "visualinfo - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "visualinfo - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "visualinfo - Win32 Debug MX" (based on "Win32 (x86) Console Application")
!MESSAGE "visualinfo - Win32 Release MX" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "visualinfo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../bin"
# PROP Intermediate_Dir "static/release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../include" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "GLEW_STATIC" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ../../lib/glew32s.lib glu32.lib opengl32.lib gdi32.lib user32.lib kernel32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "visualinfo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../bin"
# PROP Intermediate_Dir "static/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../include" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "VC_EXTRA_LEAN" /D "GLEW_STATIC" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../../lib/glew32sd.lib glu32.lib opengl32.lib gdi32.lib user32.lib kernel32.lib /nologo /subsystem:console /incremental:no /pdb:"static/debug/visualinfod.pdb" /debug /machine:I386 /out:"../../bin/visualinfod.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "visualinfo - Win32 Debug MX"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "visualinfo___Win32_Debug_MX"
# PROP BASE Intermediate_Dir "visualinfo___Win32_Debug_MX"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../bin"
# PROP Intermediate_Dir "static/debug-mx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../include" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "VC_EXTRA_LEAN" /D "GLEW_STATIC" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../include" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "VC_EXTRA_LEAN" /D "GLEW_MX" /D "GLEW_STATIC" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "GLEW_MX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ../../lib/glew32sd.lib glu32.lib opengl32.lib gdi32.lib user32.lib kernel32.lib /nologo /subsystem:console /incremental:no /pdb:"static/debug/visualinfod.pdb" /debug /machine:I386 /out:"../../bin/visualinfod.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ../../lib/glew32mxsd.lib glu32.lib opengl32.lib gdi32.lib user32.lib kernel32.lib /nologo /subsystem:console /incremental:no /pdb:"static/debug/visualinfod.pdb" /debug /machine:I386 /out:"../../bin/visualinfo-mxd.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "visualinfo - Win32 Release MX"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "visualinfo___Win32_Release_MX"
# PROP BASE Intermediate_Dir "visualinfo___Win32_Release_MX"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../bin"
# PROP Intermediate_Dir "static/release-mx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../../include" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "GLEW_STATIC" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../include" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "GLEW_MX" /D "GLEW_STATIC" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "GLEW_MX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ../../lib/glew32s.lib glu32.lib opengl32.lib gdi32.lib user32.lib kernel32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ../../lib/glew32mxs.lib glu32.lib opengl32.lib gdi32.lib user32.lib kernel32.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/visualinfo-mx.exe"

!ENDIF 

# Begin Target

# Name "visualinfo - Win32 Release"
# Name "visualinfo - Win32 Debug"
# Name "visualinfo - Win32 Debug MX"
# Name "visualinfo - Win32 Release MX"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\visualinfo.c
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\visualinfo.rc
# End Source File
# End Group
# End Target
# End Project
