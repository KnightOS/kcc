# Microsoft Developer Studio Project File - Name="sdcpp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=sdcpp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sdcpp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sdcpp.mak" CFG="sdcpp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sdcpp - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "sdcpp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sdcpp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I ".\libiberty" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /D /GZ "HAVE_CONFIG_H" /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I ".\libiberty" /I ".\win32" /I ".\libcpp" /I ".\libcpp\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "HAVE_CONFIG_H" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /out:"..\..\bin_vc\sdcpp.exe" /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386 /out:"..\..\bin_vc\sdcpp.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "sdcpp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "." /I ".\libiberty" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /I ".\libiberty" /I ".\win32" /I ".\libcpp" /I ".\libcpp\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "HAVE_CONFIG_H" /D "_WIN32" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386 /out:"..\..\bin_vc\sdcpp.exe" /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /machine:I386 /out:"..\..\bin_vc\sdcpp.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "sdcpp - Win32 Debug"
# Name "sdcpp - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "libcpp"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libcpp\charset.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\directives.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\errors.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\expr.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\files.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\identifiers.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\init.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\lex.c
# End Source File
# Begin Source File

SOURCE=".\libcpp\line-map.c"
# End Source File
# Begin Source File

SOURCE=.\libcpp\macro.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\mkdeps.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\symtab.c
# End Source File
# Begin Source File

SOURCE=.\libcpp\traditional.c
# End Source File
# End Group
# Begin Group "libiberty"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libiberty\concat.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\fopen_unlocked.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\getpwd.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\hashtab.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\hex.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\lbasename.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\md5.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\obstack.c
# End Source File
# Begin Source File

SOURCE=".\libiberty\safe-ctype.c"
# End Source File
# Begin Source File

SOURCE=".\libiberty\splay-tree.c"
# End Source File
# Begin Source File

SOURCE=.\libiberty\vasprintf.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\xexit.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\xmalloc.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\xmemdup.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\xstrdup.c
# End Source File
# Begin Source File

SOURCE=.\libiberty\xstrerror.c
# End Source File
# End Group
# Begin Group "win32"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\win32\dirent.c
# End Source File
# End Group
# Begin Source File

SOURCE=".\c-incpath.c"
# End Source File
# Begin Source File

SOURCE=".\c-ppoutput.c"
# End Source File
# Begin Source File

SOURCE=.\cppdefault.c
# End Source File
# Begin Source File

SOURCE=.\options.c
# End Source File
# Begin Source File

SOURCE=".\opts-common.c"
# End Source File
# Begin Source File

SOURCE=.\opts.c
# End Source File
# Begin Source File

SOURCE=.\prefix.c
# End Source File
# Begin Source File

SOURCE=".\sdcpp-diagnostic.c"
# End Source File
# Begin Source File

SOURCE=".\sdcpp-opts.c"
# End Source File
# Begin Source File

SOURCE=.\sdcpp.c
# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "libcpp_h"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "include"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=".\libcpp\include\cpp-id-data.h"
# End Source File
# Begin Source File

SOURCE=.\libcpp\include\cpplib.h
# End Source File
# Begin Source File

SOURCE=".\libcpp\include\line-map.h"
# End Source File
# Begin Source File

SOURCE=.\libcpp\include\mkdeps.h
# End Source File
# Begin Source File

SOURCE=.\libcpp\include\symtab.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\libcpp\internal.h
# End Source File
# Begin Source File

SOURCE=.\libcpp\system.h
# End Source File
# Begin Source File

SOURCE=.\libcpp\ucnid.h
# End Source File
# End Group
# Begin Group "win32_h"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\win32\dirent.h
# End Source File
# End Group
# Begin Group "libiberty_h"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\libiberty\filenames.h
# End Source File
# Begin Source File

SOURCE=.\libiberty\hashtab.h
# End Source File
# Begin Source File

SOURCE=.\libiberty\obstack.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ansidecl.h
# End Source File
# Begin Source File

SOURCE=".\auto-host.h"
# End Source File
# Begin Source File

SOURCE=".\c-incpath.h"
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\cppdefault.h
# End Source File
# Begin Source File

SOURCE=.\hwint.h
# End Source File
# Begin Source File

SOURCE=.\intl.h
# End Source File
# Begin Source File

SOURCE=.\libiberty.h
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\opts.h
# End Source File
# Begin Source File

SOURCE=.\output.h
# End Source File
# Begin Source File

SOURCE=.\prefix.h
# End Source File
# Begin Source File

SOURCE=".\libiberty\safe-ctype.h"
# End Source File
# Begin Source File

SOURCE=.\sdcpp.h
# End Source File
# Begin Source File

SOURCE=".\libiberty\splay-tree.h"
# End Source File
# Begin Source File

SOURCE=.\symcat.h
# End Source File
# Begin Source File

SOURCE=.\system.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# End Target
# End Project
