# This Makefile is in the public domain.

CC = gcc

CFLAGS = -w $(CDEBUG) -DHARDCODE -DSYSV $(OPT)
CDEBUG = -g
LIBS =
EXE = vt

OBJECTS = util.o sconst.o prmtab.o console.o signal.o keyboard.o \
	  interp.o remote.o oper.o func.o string.o unode.o prmt3.o \
	  array.o bobj.o key.o main.o prmt1.o prmt2.o prmt4.o prmt5.o \
	  const.o var.o vtc.o window.o regexp.o tmalloc.o telnet.o

all: vt

vt: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(EXE) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS) $(EXE)

$(OBJECTS): vt.h struct.h string.h config.h extern.h

