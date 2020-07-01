# Microsoft Developer Studio Generated NMAKE File, Based on TabCtrl.dsp
!IF "$(CFG)" == ""
CFG=TabCtrl - Win32 Debug
!MESSAGE No configuration specified. Defaulting to TabCtrl - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "TabCtrl - Win32 Release" && "$(CFG)" != "TabCtrl - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TabCtrl.mak" CFG="TabCtrl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TabCtrl - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TabCtrl - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TabCtrl - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\TabCtrl.exe"


CLEAN :
	-@erase "$(INTDIR)\CompPg.obj"
	-@erase "$(INTDIR)\ExpPag.obj"
	-@erase "$(INTDIR)\OutPg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TabCtrl.obj"
	-@erase "$(INTDIR)\TabCtrl.pch"
	-@erase "$(INTDIR)\TabCtrl.res"
	-@erase "$(INTDIR)\TabCtrlDlg.obj"
	-@erase "$(INTDIR)\TabSheet.obj"
	-@erase "$(INTDIR)\TCIni.obj"
	-@erase "$(INTDIR)\TCLog.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\TabCtrl.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\TabCtrl.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\TabCtrl.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\TabCtrl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\TabCtrl.pdb" /machine:I386 /out:"$(OUTDIR)\TabCtrl.exe" 
LINK32_OBJS= \
	"$(INTDIR)\CompPg.obj" \
	"$(INTDIR)\ExpPag.obj" \
	"$(INTDIR)\OutPg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TabCtrl.obj" \
	"$(INTDIR)\TabCtrlDlg.obj" \
	"$(INTDIR)\TabSheet.obj" \
	"$(INTDIR)\TCLog.obj" \
	"$(INTDIR)\TabCtrl.res" \
	"$(INTDIR)\TCIni.obj"

"$(OUTDIR)\TabCtrl.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "TabCtrl - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\TabCtrl.exe"


CLEAN :
	-@erase "$(INTDIR)\CompPg.obj"
	-@erase "$(INTDIR)\ExpPag.obj"
	-@erase "$(INTDIR)\OutPg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TabCtrl.obj"
	-@erase "$(INTDIR)\TabCtrl.pch"
	-@erase "$(INTDIR)\TabCtrl.res"
	-@erase "$(INTDIR)\TabCtrlDlg.obj"
	-@erase "$(INTDIR)\TabSheet.obj"
	-@erase "$(INTDIR)\TCIni.obj"
	-@erase "$(INTDIR)\TCLog.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\TabCtrl.exe"
	-@erase "$(OUTDIR)\TabCtrl.ilk"
	-@erase "$(OUTDIR)\TabCtrl.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\TabCtrl.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\TabCtrl.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\TabCtrl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\TabCtrl.pdb" /debug /machine:I386 /out:"$(OUTDIR)\TabCtrl.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\CompPg.obj" \
	"$(INTDIR)\ExpPag.obj" \
	"$(INTDIR)\OutPg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TabCtrl.obj" \
	"$(INTDIR)\TabCtrlDlg.obj" \
	"$(INTDIR)\TabSheet.obj" \
	"$(INTDIR)\TCLog.obj" \
	"$(INTDIR)\TabCtrl.res" \
	"$(INTDIR)\TCIni.obj"

"$(OUTDIR)\TabCtrl.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("TabCtrl.dep")
!INCLUDE "TabCtrl.dep"
!ELSE 
!MESSAGE Warning: cannot find "TabCtrl.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "TabCtrl - Win32 Release" || "$(CFG)" == "TabCtrl - Win32 Debug"
SOURCE=.\CompPg.cpp

"$(INTDIR)\CompPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"


SOURCE=.\ExpPag.cpp

"$(INTDIR)\ExpPag.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"


SOURCE=.\OutPg.cpp

"$(INTDIR)\OutPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "TabCtrl - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\TabCtrl.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\TabCtrl.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "TabCtrl - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\TabCtrl.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\TabCtrl.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TabCtrl.cpp

"$(INTDIR)\TabCtrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"


SOURCE=.\TabCtrl.rc

"$(INTDIR)\TabCtrl.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\TabCtrlDlg.cpp

"$(INTDIR)\TabCtrlDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"


SOURCE=.\TabSheet.cpp

"$(INTDIR)\TabSheet.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"


SOURCE=.\TCIni.cpp

"$(INTDIR)\TCIni.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"


SOURCE=.\TCLog.cpp

"$(INTDIR)\TCLog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\TabCtrl.pch"



!ENDIF 

