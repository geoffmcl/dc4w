# Microsoft Developer Studio Project File - Name="izuz" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=izuz - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "izuz.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "izuz.mak" CFG="izuz - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "izuz - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "izuz - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "izuz - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /MT  /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "GMUZ" /D "IZDLL" /D "NO_ZIPINFO" /D "_CRT_SECURE_NO_DEPRECATE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo 

!ELSEIF  "$(CFG)" == "izuz - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /MTd  /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "WIN32" /D "_DEBUG" /D "_LIB" /D "GMUZ" /D "IZDLL" /D "NO_ZIPINFO" /D "_CRT_SECURE_NO_DEPRECATE" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo 

!ENDIF 

# Begin Target

# Name "izuz - Win32 Release"
# Name "izuz - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cc;cxx;def;odl;idl;hpj;bat;asm;asmx"
# Begin Source File

SOURCE=.\iz_api.c
# End Source File
# Begin Source File

SOURCE=.\iz_crc32.c
# End Source File
# Begin Source File

SOURCE=.\iz_crctab.c
# End Source File
# Begin Source File

SOURCE=.\iz_crpt.c
# End Source File
# Begin Source File

SOURCE=.\iz_envargs.c
# End Source File
# Begin Source File

SOURCE=.\iz_explode.c
# End Source File
# Begin Source File

SOURCE=.\iz_extract.c
# End Source File
# Begin Source File

SOURCE=.\iz_fileio.c
# End Source File
# Begin Source File

SOURCE=.\iz_globals.c
# End Source File
# Begin Source File

SOURCE=.\iz_inflate.c
# End Source File
# Begin Source File

SOURCE=.\iz_list.c
# End Source File
# Begin Source File

SOURCE=.\iz_match.c
# End Source File
# Begin Source File

SOURCE=.\iz_nt.c
# End Source File
# Begin Source File

SOURCE=.\iz_process.c
# End Source File
# Begin Source File

SOURCE=.\iz_ttyio.c
# End Source File
# Begin Source File

SOURCE=.\iz_unzip.c
# End Source File
# Begin Source File

SOURCE=.\iz_win32.c
# End Source File
# Begin Source File

SOURCE=.\iz_zipinfo.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc;xsd"
# Begin Source File

SOURCE=.\iz_consts.h
# End Source File
# Begin Source File

SOURCE=.\iz_crypt.h
# End Source File
# Begin Source File

SOURCE=.\iz_ebcdic.h
# End Source File
# Begin Source File

SOURCE=.\iz_globals.h
# End Source File
# Begin Source File

SOURCE=.\iz_inflate.h
# End Source File
# Begin Source File

SOURCE=.\iz_nt.h
# End Source File
# Begin Source File

SOURCE=.\iz_ttyio.h
# End Source File
# Begin Source File

SOURCE=.\iz_unzip.h
# End Source File
# Begin Source File

SOURCE=.\iz_unzpriv.h
# End Source File
# Begin Source File

SOURCE=.\iz_userfun.h
# End Source File
# Begin Source File

SOURCE=.\iz_version.h
# End Source File
# Begin Source File

SOURCE=.\iz_w32cfg.h
# End Source File
# Begin Source File

SOURCE=.\iz_zip.h
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Group
# End Target
# End Project
