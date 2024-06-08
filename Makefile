CC = g++
INC = -I. -I../../heaplayers -I../../heaplayers/util
# CM_CFLAGS = -pipe -malign-double -mcpu=pentium4 -mtune=pentium4 -shared -Wno-invalid-offsetof
CM_CFLAGS = -pipe -shared -Wno-invalid-offsetof
DB_CFLAGS = -g -DDEBUG -DMYASSERT
OP_CFLAGS = -O3 -UDEBUG -DNDEBUG

all:	vam

clean:
	rm -f *.o *.so
vam_debug:
	$(CC) $(INC) $(CM_CFLAGS) $(DB_CFLAGS) libvam.cpp -o libvam_debug.so -ldl
vam:
	$(CC) $(INC) $(CM_CFLAGS) $(OP_CFLAGS) libvam.cpp -o libvam.so -ldl
vam_inline:
	$(CC) $(INC) $(CM_CFLAGS) $(OP_CFLAGS) libvam.cpp -o libvam_inline.so -finline-limit=65000 -ldl
vam_discard:
	$(CC) $(INC) $(CM_CFLAGS) $(OP_CFLAGS) libvam.cpp -o libvam.so -DAGGRESSIVE_DISCARD -ldl
vam_trace:
	$(CC) $(INC) $(CM_CFLAGS) $(OP_CFLAGS) libvam.cpp -o libvam_trace.so -DAGGRESSIVE_DISCARD -DMEMORY_TRACE -ldl
all: vam_debug vam vam_inline vam_discard vam_trace
