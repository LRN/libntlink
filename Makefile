EXESUF = exe
SOSUF = dll
ASUF = a
INC_CFLAGS = $(CFLAGS)
LIB_LDFLAGS = $(LDFLAGS)
CC = gcc
SPEC_CFLAGS = $(ASM_CFLAGS) -I.
ifdef DEBUG
DEBUG_CFLAGS = $(SPEC_CFLAGS) -O0 -g
else
DEBUG_CFLAGS = $(SPEC_CFLAGS) -O3
endif
LOCAL_CFLAGS = $(DEBUG_CFLAGS) -fno-common -Wall -mms-bitfields -D_WIN32_WINNT=0x600
NTLINK_SHARED = libntlink.$(SOSUF)
NTLINK_STATIC = libntlink.$(ASUF)
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

$(NTLINK_SHARED): $(NTLINK_OBJECT_FILES)
ifeq ($(ENV),mingw-cmd)
	$(CC) -shared  $(NTLINK_OBJECT_FILES) $(LIB_LDFLAGS) -Wl,-soname,$(SONAME) -o $(NTLINK_SHARED) -Wl,--out-implib=$(CURDIR)\$(NTLINK_STATIC)
else
	$(CC) -shared  $(NTLINK_OBJECT_FILES) $(LIB_LDFLAGS) -Wl,-soname,$(SONAME) -o $(NTLINK_SHARED) -Wl,--out-implib=$(shell pwd -W)/$(NTLINK_STATIC)
endif

$(JUNC_NAME): $(NTLINK_STATIC) $(JUNC_OBJECT_FILES)
ifeq ($(ENV),mingw-cmd)
	$(CC) -static -o $(JUNC_NAME) $(JUNC_OBJECT_FILES) $(LIB_LDFLAGS) -L$(CURDIR) -lntlink
else
	$(CC) -static -o $(JUNC_NAME) $(JUNC_OBJECT_FILES) $(LIB_LDFLAGS) -L$(shell pwd -W) -lntlink
endif

install: $(NTLINK_SHARED) $(JUNC_NAME)
ifndef PREFIX
ifeq ($(ENV),mingw-cmd)
	@echo Please use PREFIX=drive:\installation\directory to specify installation path
else
	@echo Please use PREFIX=/installation/directory to specify installation path
endif
else
ifeq ($(ENV),mingw-cmd)
	@echo Installing as $(PREFIX)\$(NTLINK_SHARED)
	cmd /C if NOT EXIST $(PREFIX) mkdir $(PREFIX)
	cmd /C copy /Y $(NTLINK_SHARED) $(PREFIX)\$(NTLINK_SHARED)
else
	@echo Installing as $(PREFIX)/$(NTLINK_SHARED)
	install -D $(NTLINK_SHARED) $(PREFIX)/$(NTLINK_SHARED)
endif
endif
