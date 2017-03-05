# Makefile for Meteo Sarvar
LOG_FACILITY = LOG_LOCAL0

DEFINES = -DLOG_FACILITY="$(LOG_FACILITY)"
CFLAGS += $(DEFINES)
LDLIBS += -lm -pthread

mts_comm: mts_comm.o
	    $(CC) $(LDFLAGS) mts_comm.o $(LDLIBS) -o mts_comm

mts_comm.o: mts_comm.c
	    $(CC) $(CFLAGS) -c mts_comm.c

convert.o: convert.c
	    clang -c convert.c

files.o: files.c
	    clang -c files.c

clean:
	    rm -f *.o mts_comm

test: test.c
#	rm -f test
	$(CC) -o test test.c