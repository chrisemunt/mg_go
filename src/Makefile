# UNIX makefile for mg_dba module (mg_dba.so)
#
# Build:
# make Makefile
#
# Cleanup:
# make clean
#
# Install:
# make install


MGDBAINC=./

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
   MGDBAOS = Linux
   CC = cc
   CCFLAGS += -fpic -DLINUX
   LINK += cc
   LINKFLAGS += -shared -rdynamic
endif
ifeq ($(UNAME_S),Darwin)
   MGDBAOS = OSX
   CC = cc
   CCFLAGS += -fPIC -fno-common -DMACOSX -D_NOTHREADS -DDARWIN
   LINK += cc
   LINKFLAGS += -bundle -flat_namespace -undefined suppress
endif
ifeq ($(UNAME_S),FreeBSD)
   MGDBAOS = FreeBSD
   CC = cc
   CCFLAGS += -DFREEBSD
   LINK = ld
   LINKFLAGS += -G
endif
ifeq ($(UNAME_S),AIX)
   MGDBAOS = AIX
   CC = xlc_r
   CCFLAGS += -DAIX
   LINK = xlc_r
   LINKFLAGS += -G -H512 -T512 -bM:SRE -berok -bexpall -bnoentry
endif

ifndef MGDBAOS
   $(error Cannot determine the OS type)
endif

# Build mg_dba module
all : app

# Link the object file and dependent libraries into a binary
app : mg_dba.o
	$(LINK) -o mg_dba.so $(LINKFLAGS) mg_dba.o

# Compile the source file into object file
mg_dba.o : mg_dba.c
	$(CC) -o mg_dba.o $(CCFLAGS) -I$(MGDBAINC) -c mg_dba.c

# Install mg_dba module
install :
	$(error Copy mg_dba.so to a location accessible to Go)

# Clean target
clean :
	rm -rf mg_dba.so mg_dba.o



