# Makefile for Meteo Sarvar

mts_comm: mts_comm.o convert.o files.o
	    clang -pthread -o mts_comm mts_comm.o convert.o files.o

mts_comm.o: mts_comm.c
	    clang -c mts_comm.c
	    
convert.o: convert.c
	    clang -c convert.c
	    
files.o: files.c
	    clang -c files.c

clean:
	    rm -f *.o mts_comm

test:
	rm -f test
	clang -pthread -o test test.c