CC = gcc
CFLAGS = -gdwarf-2

libmultiverse.a: mv_info.o mv_commit.o arch-x86.o
	ar rcs $@ $^

mv_info.o: mv_info.c multiverse.h

clean:
	rm -f *.o libmultiverse.a
