CC = g++
INC = -I. -I.. -I../../../heaplayers -I../../../heaplayers/util
CM_CFLAGS = -pipe -mcpu=pentium4 -mtune=pentium4 -D_REENTRANT=1 -lpthread
DB_CFLAGS = -g -DDEBUG -DMYASSERT
OP_CFLAGS = -O3 -UDEBUG -DNDEBUG

all: memtrace malloctrace lrusim

clean:
	rm -f *.o *.so

memtrace_debug:
	$(CC) $(INC) $(CM_CFLAGS) $(DB_CFLAGS) -shared libmemtrace.cpp -o libmemtrace.so -ldl

memtrace:
	$(CC) $(INC) $(CM_CFLAGS) $(OP_CFLAGS) -shared libmemtrace.cpp -o libmemtrace.so -ldl

malloctrace:
	$(CC) $(INC) $(CM_CFLAGS) $(OP_CFLAGS) -shared libmalloctrace.cpp -o libmalloctrace.so -ldl

lrusim:
	$(CC) $(CM_CFLAGS) $(OP_CFLAGS) lrusim.cpp -o lrusim

lrusim2:
	$(CC) $(CM_CFLAGS) $(OP_CFLAGS) lrusim2.cpp -o lrusim2

lrusim3:
	$(CC) $(CM_CFLAGS) $(OP_CFLAGS) lrusim3.cpp -o lrusim3
