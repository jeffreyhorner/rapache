# Microsoft Developer Studio Generated NMAKE File, Based on testall.dsp

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
CFG=test_cgi - Win32 Release
!MESSAGE No configuration specified. Defaulting to test_cgi - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "test_cgi - Win32 Release" && "$(CFG)" != "test_cgi - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "test_cgi.mak" CFG="test_cgi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "test_cgi - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "test_cgi - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF

CFG_HOME=$(APREQ_HOME)\win32
MODULE=$(APREQ_HOME)\module
OUTDIR=$(CFG_HOME)\libs
INTDIR=$(CFG_HOME)\libs

LINK32_OBJS= \
        "$(INTDIR)\test_cgi.obj" \
	"$(OUTDIR)\libapreq2.lib" \
	"$(APR_LIB)" \
	"$(APU_LIB)"

!IF  "$(CFG)" == "test_cgi - Win32 Release"
ALL : "$(OUTDIR)\test_cgi.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /I"$(APACHE)\include" /I"$(APREQ_HOME)\include" /Fp"$(INTDIR)\test_cgi.pch" /YX /FD /c 

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\test_cgi.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib wsock32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\test_cgi.pdb" /machine:I386 /out:"$(MODULE)\t\cgi-bin\test_cgi.exe" 

!ELSEIF  "$(CFG)" == "test_cgi - Win32 Debug"

ALL : "$(OUTDIR)\test_cgi.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /I"$(APACHE)\include" /I"$(APREQ_HOME)\include" /YX /FD /GZ /c 

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\test_cgi.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib wsock32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\test_cgi.pdb" /debug /machine:I386 /out:"$(MODULE)\t\cgi-bin\test_cgi.exe" /pdbtype:sept 

!ENDIF 

!IF "$(CFG)" == "test_cgi - Win32 Release" || "$(CFG)" == "test_cgi - Win32 Debug"

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

"$(OUTDIR)\test_cgi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(MODULE)\test_cgi.c

"$(INTDIR)\test_cgi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(INTDIR)\test_cgi.obj" $(CPP_PROJ) $(SOURCE)

!ENDIF 

