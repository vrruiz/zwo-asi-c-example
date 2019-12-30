CC=gcc
CFLAGS=-I../include/
LD=../lib/x64/
LDIR=-L$(LD)
LIBS=-lASICamera2

asi-example: asi-example.c
	$(CC) $? -o $@ $(CFLAGS) $(LIBS) $(LDIR)

run:
	LD_LIBRARY_PATH=${LD} ./test-asi
