CFLAGS += -gdwarf-2
libmultiverse.a: mv_info.o
	ar rcs $@ $^

mv_info.o: mv_info.c multiverse.h

clean:
	rm -f *.o libmultiverse.a
