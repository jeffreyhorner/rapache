# Microsoft Developer Studio Generated NMAKE File, Based on mod_apreq.dsp

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please use Perl Makefile.PL, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(APR_LIB)" == ""
!MESSAGE No apr lib was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please use Perl Makefile.PL, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(APU_LIB)" == ""
!MESSAGE No aprutil lib was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please use Perl Makefile.PL, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(CFG)" == ""
CFG=mod_apreq2 - Win32 Release
!MESSAGE No configuration specified. Defaulting to mod_apreq2 - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "mod_apreq2 - Win32 Release" && "$(CFG)" != "mod_apreq2 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mod_apreq2.mak" CFG="mod_apreq2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mod_apreq2 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mod_apreq2 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

CFG_HOME=$(APREQ_HOME)\win32
OUTDIR=$(CFG_HOME)\libs
INTDIR=$(CFG_HOME)\libs
MODDIR=$(APREQ_HOME)\module\apache2

LINK32_OBJS= \
	"$(INTDIR)\handle.obj" \
	"$(INTDIR)\filter.obj" \
	"$(APR_LIB)" \
	"$(APU_LIB)" \
	"$(APACHE)\lib\libhttpd.lib" \
	"$(OUTDIR)\libapreq2.lib"


!IF  "$(CFG)" == "mod_apreq2 - Win32 Release"

ALL : "$(OUTDIR)\mod_apreq2.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MOD_APREQ_EXPORTS" /I"$(APACHE)\include" /I"$(APREQ_HOME)\include" /I"$(MODDIR)" /YX /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mod_apreq2.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /machine:I386 /out:"$(OUTDIR)\mod_apreq2.so" /implib:"$(OUTDIR)\mod_apreq2.lib" 
"$(OUTDIR)\mod_apreq2.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mod_apreq2 - Win32 Debug"

ALL : "$(OUTDIR)\mod_apreq2.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MOD_APREQ_EXPORTS" /I"$(APACHE)\include" /I"$(APREQ_HOME)\include" /I"$(MODDIR)" /YX /FD /GZ  /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mod_apreq2.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\mod_apreq2.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mod_apreq2.so" /implib:"$(OUTDIR)\mod_apreq2.lib" /pdbtype:sept 
"$(OUTDIR)\mod_apreq2.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "mod_apreq2 - Win32 Release" || "$(CFG)" == "mod_apreq2 - Win32 Debug"

SOURCE=$(MODDIR)\filter.c

"$(INTDIR)\filter.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(INTDIR)\filter.obj" $(CPP_PROJ) $(SOURCE)

SOURCE=$(MODDIR)\handle.c

"$(INTDIR)\handle.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(INTDIR)\handle.obj" $(CPP_PROJ) $(SOURCE)

!ENDIF 

