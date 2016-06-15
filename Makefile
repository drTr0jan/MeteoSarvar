# Makefile for Meteo Sarvar

mts_comm: mts_comm.o convert.o filename.o
	    clang -o mts_comm mts_comm.o convert.o filename.o
	    
mts_comm.o: mts_comm.c
	    clang -c mts_comm.c
	    
convert.o: convert.c
	    clang -c convert.c
	    
filename.o: filename.c
	    clang -c filename.c

clean:
	    rm -f *.o mts_comm
