# Microsoft Developer Studio Generated NMAKE File, Based on apreq2_test.dsp
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
CFG=apreq2_test - Win32 Release
!MESSAGE No configuration specified. Defaulting to apreq2_test - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "apreq2_test - Win32 Release" && "$(CFG)" != "apreq2_test - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "apreq2_test.mak" CFG="apreq2_test - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "apreq2_test - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "apreq2_test - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

CFG_HOME=$(APREQ_HOME)\win32
LIBTDIR=$(APREQ_HOME)\library\t
OUTDIR=$(LIBTDIR)
INTDIR=$(LIBTDIR)
LIBDIR=$(CFG_HOME)\libs
PROGRAMS="$(LIBTDIR)\params.exe" "$(LIBTDIR)\version.exe" \
  "$(LIBTDIR)\parsers.exe" "$(LIBTDIR)\cookie.exe"

!IF  "$(CFG)" == "apreq2_test - Win32 Release"

ALL : "$(LIBDIR)\apreq2_test.lib" $(PROGRAMS)

CLEAN :
	-@erase "$(INTDIR)\at.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\apreq2_test.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /I"$(APACHE)\include" /I"$(APREQ_HOME)\include" /YX /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(LIBDIR)\apreq2_test.bsc"	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(LIBDIR)\apreq2_test.lib" 
LIB32_OBJS= \
	"$(LIBDIR)\at.obj" \
	"$(APR_LIB)" \
	"$(APU_LIB)"

"$(LIBDIR)\apreq2_test.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib wsock32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes  /debug /machine:I386 /pdbtype:sept 

LINK32_OBJS= \
	"$(LIBDIR)\libapreq2.lib" \
	"$(LIBDIR)\apreq2_test.lib" \
	"$(APR_LIB)" \
	"$(APU_LIB)" \
	"$(APACHE)\lib\libhttpd.lib"

"$(LIBTDIR)\cookie.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
    $(LINK32) /pdb:"$(LIBTDIR)\cookie.pdb" /out:"$(LIBTDIR)\cookie.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
<<

"$(LIBTDIR)\params.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
    $(LINK32) /pdb:"$(LIBTDIR)\params.pdb" /out:"$(LIBTDIR)\params.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
<<

"$(LIBTDIR)\parsers.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\parsers.obj"
    $(LINK32) /pdb:"$(LIBTDIR)\parsers.pdb" /out:"$(LIBTDIR)\parsers.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\parsers.obj"
<<

"$(LIBTDIR)\version.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
    $(LINK32) /pdb:"$(LIBTDIR)\version.pdb" /out:"$(LIBTDIR)\version.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
<<

!ELSEIF  "$(CFG)" == "apreq2_test - Win32 Debug"

ALL : "$(OUTDIR)\apreq2_test.lib" $(PROGRAMS)

CLEAN :
	-@erase "$(INTDIR)\at.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\apreq2_test.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB"/YX /I"$(APACHE)\include" /I"$(APREQ_HOME)\include" /FD /GZ  /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(LIBDIR)\apreq2_test.bsc" 
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(LIBDIR)\apreq2_test.lib" 
LIB32_OBJS= \
	"$(LIBDIR)\at.obj" \
	"$(APR_LIB)" \
	"$(APU_LIB)"

"$(LIBDIR)\apreq2_test.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

"$(LIBTDIR)\cookie.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
    $(LINK32) /pdb:"$(LIBTDIR)\cookie.pdb" /out:"$(LIBTDIR)\cookie.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
<<

"$(LIBTDIR)\params.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
    $(LINK32) /pdb:"$(LIBTDIR)\params.pdb" /out:"$(LIBTDIR)\params.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
<<

"$(LIBTDIR)\parser.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\parser.obj"
    $(LINK32) /pdb:"$(TESTFILE)\parser.pdb" /out:"$(LIBTDIR)\parser.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\parser.obj"
<<

"$(LIBTDIR)\version.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
    $(LINK32) /pdb:"$(LIBTDIR)\version.pdb" /out:"$(LIBTDIR)\version.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
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

!IF "$(CFG)" == "apreq2_test - Win32 Release" || "$(CFG)" == "apreq2_test - Win32 Debug"

SOURCE=$(LIBTDIR)\at.c

"$(LIBDIR)\at.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(LIBDIR)\at.obj" $(CPP_PROJ) $(SOURCE)

SOURCE=$(LIBTDIR)\cookie.c

"$(OUTDIR)\cookie.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(OUTDIR)\cookie.obj" $(CPP_PROJ) $(SOURCE)

SOURCE=$(LIBTDIR)\params.c

"$(OUTDIR)\params.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(OUTDIR)\params.obj" $(CPP_PROJ) $(SOURCE)

SOURCE=$(LIBTDIR)\parsers.c

"$(OUTDIR)\parsers.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(OUTDIR)\parsers.obj" $(CPP_PROJ) $(SOURCE)

SOURCE=$(LIBTDIR)\version.c

"$(OUTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) /Fo"$(OUTDIR)\version.obj" $(CPP_PROJ) $(SOURCE)

!ENDIF 

