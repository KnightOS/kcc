# Microsoft Developer Studio Project File - Name="z80a" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=z80a - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "z80a.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "z80a.mak" CFG="z80a - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "z80a - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G3 /ML /W3 /WX /Gm /GX /ZI /Od /I ".." /I "." /I "..\.." /I "..\..\support\util" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /J /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"port.lib"
# Begin Target

# Name "z80a - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=".\peeph-gbz80.def"
# Begin Custom Build
InputPath=".\peeph-gbz80.def"

"peeph-gbz80.rul" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo on 
	gawk -f ../SDCCpeeph.awk $(InputPath) >peeph-gbz80.rul 
	
# End Custom Build
# End Source File
# Begin Source File

SOURCE=".\peeph-z80.def"
# Begin Custom Build
InputPath=".\peeph-z80.def"

"peeph-z80.rul" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo on 
	gawk -f ../SDCCpeeph.awk $(InputPath) >peeph-z80.rul 
	
# End Custom Build
# End Source File
# Begin Source File

SOURCE=".\peeph.def"
# Begin Custom Build
InputPath=".\peeph.def"

"peeph.rul" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo on 
	gawk -f ../SDCCpeeph.awk $(InputPath) >peeph.rul 
	
# End Custom Build
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
