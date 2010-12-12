EXESUF = exe
SOSUF = dll
ASUF = a
INC_CFLAGS = $(CFLAGS)
ifeq ($(WINDIR),)
	ifeq ($(windir),)
        	WDIR=
	else
        	WDIR=$(windir)
	endif
        
else
        WDIR=$(WINDIR)
endif
LIB_LDFLAGS = -L$(WDIR)/system32 $(LDFLAGS)
LIB_LIBS = -lkernel32 $(LIBS)
CC = gcc
AR = ar
SPEC_CFLAGS = $(ASM_CFLAGS) -I.
ifdef DEBUG
DEBUG_CFLAGS = $(SPEC_CFLAGS) -O0 -g
else
DEBUG_CFLAGS = $(SPEC_CFLAGS) -O3
endif
LOCAL_CFLAGS = $(DEBUG_CFLAGS) -fno-common -Wall -mms-bitfields -D_WIN32_WINNT=0x600
NTLINK_SHARED = libntlink.$(SOSUF)
NTLINK_STATIC = libntlink.$(ASUF)
NTLINK_IMPORT = libntlink.$(SOSUF).$(ASUF)
JUNC_NAME = junc.$(EXESUF)
NTLINK_FILES = juncpoint.c quasisymlink.c misc.c extra_string.c
NTLINK_HEADERS = quasisymlink.h juncpoint.h misc.h extra_string.h
JUNC_FILES = junc.c
NTLINK_OBJECT_FILES = $(patsubst %.c,%.o,$(NTLINK_FILES))
JUNC_OBJECT_FILES = $(patsubst %.c,%.o,$(JUNC_FILES))
CD=$(shell cd)

ifeq ($(OS),Windows_NT)
	ifeq ($(MSYSTEM),MINGW32)
		ENV = mingw-msys
	else
		ENV = mingw-cmd
	endif
else
	ENV = gnu
endif

all: $(NTLINK_SHARED) $(JUNC_NAME)

clean:
ifeq ($(ENV),mingw-cmd)
	cmd /c "del *.o *.dll *.a *.exe"
else
	rm *.o *.dll *.a *.exe
endif

%.o: %.c
	$(CC) $(LOCAL_CFLAGS) -o $@ -c $<

$(NTLINK_IMPORT): $(NTLINK_SHARED)

$(NTLINK_SHARED): $(NTLINK_OBJECT_FILES)
ifeq ($(ENV),mingw-cmd)
	$(CC) -shared  $(NTLINK_OBJECT_FILES) $(LIB_LDFLAGS) -o $(NTLINK_SHARED) -Wl,--out-implib=$(CURDIR)\$(NTLINK_IMPORT) $(LIB_LIBS)
else
	$(CC) -shared  $(NTLINK_OBJECT_FILES) $(LIB_LDFLAGS) -o $(NTLINK_SHARED) -Wl,--out-implib=$(shell "pwd" "-W")/$(NTLINK_IMPORT) $(LIB_LIBS)
endif

$(NTLINK_STATIC): $(NTLINK_OBJECT_FILES)
	$(AR) rcs $(NTLINK_STATIC) $(NTLINK_OBJECT_FILES)

$(JUNC_NAME): $(NTLINK_STATIC) $(JUNC_OBJECT_FILES)
ifeq ($(ENV),mingw-cmd)
	$(CC) -o $(JUNC_NAME) $(JUNC_OBJECT_FILES) $(LIB_LDFLAGS) $(DIRECT_DLL_LDFLAGS) -L$(CURDIR) -lntlink
else
	$(CC) -o $(JUNC_NAME) $(JUNC_OBJECT_FILES) $(LIB_LDFLAGS) $(DIRECT_DLL_LDFLAGS) -L$(shell "pwd" "-W") -lntlink
endif

install: $(NTLINK_SHARED) $(NTLINK_IMPORT) $(NTLINK_STATIC) $(JUNC_NAME)
ifndef DESTDIR
ifeq ($(ENV),mingw-cmd)
	@echo Please use DESTDIR=drive:\installation\directory to specify installation path
else
	@echo Please use DESTDIR=/installation/directory to specify installation path
endif
else
ifeq ($(ENV),mingw-cmd)
	@echo Installing as $(DESTDIR)\bin\$(NTLINK_SHARED)
	cmd /C if NOT EXIST $(DESTDIR)\bin mkdir $(DESTDIR)\bin
	cmd /C if NOT EXIST $(DESTDIR)\lib mkdir $(DESTDIR)\lib
	cmd /C if NOT EXIST $(DESTDIR)\include\libntlink mkdir $(DESTDIR)\include\libntlink
	cmd /C copy /Y $(NTLINK_SHARED) $(DESTDIR)\bin\$(NTLINK_SHARED)
	cmd /C copy /Y $(NTLINK_STATIC) $(DESTDIR)\lib\$(NTLINK_STATIC)
	cmd /C copy /Y $(NTLINK_IMPORT) $(DESTDIR)\lib\$(NTLINK_IMPORT)
	cmd /C for %i in ($(NTLINK_HEADERS)) do copy /Y %i $(DESTDIR)\include\libntlink
else
	@echo Installing as $(DESTDIR)/bin/$(NTLINK_SHARED)
	mkdir -p $(DESTDIR)/bin
	mkdir -p $(DESTDIR)/lib
	mkdir -p $(DESTDIR)/include/libntlink
	install -D $(NTLINK_SHARED) $(DESTDIR)/bin/$(NTLINK_SHARED)
	install -D $(NTLINK_STATIC) $(DESTDIR)/lib/$(NTLINK_STATIC)
	install -D $(NTLINK_IMPORT) $(DESTDIR)/lib/$(NTLINK_IMPORT)
	install -D $(NTLINK_HEADERS) $(DESTDIR)/include/libntlink
endif
endif
