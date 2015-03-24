CC = gcc
CFLAGS+=-Wall -Werror -std=c99
LDLIBS+= -lpthread -lprussdrv -L./lib -lpru1aio
INLDLIBS+= -lpthread -lprussdrv -lpru1aio
DEPS = pru1aio.p pru0aio.p pru1aio.hp
OBJ = main.o

all: pru1aio.bin pru0aio.bin clearpru.bin libpru1aio.a lib/libpru1aio.so.1.0.0 pru1aio pru1aio

clean:
	rm -f pru1aio *.o *.bin *.csv *.a *_bin.h
	rm lib/*

clearpru.bin: clearpru.p
	#pasm -b $^
	pasm -CPRUcode_clear $^
	
pru0aio.bin: pru0aio.p
	#pasm -b $^
	pasm -CPRUcode_pru0 $^

pru1aio.bin: pru1aio.p
	#pasm -b $^
	pasm -CPRUcode_pru1 $^

pru1aio.o: pru1aio.c $(DEPS)
	$(CC) $(CFLAGS) -static -c -o pru1aio.o pru1aio.c
	
libpru1aio.a: pru1aio.o
	install -D -t /usr/include ./include/pru1aio.h
	ar rcs libpru1aio.a pru1aio.o
	mv libpru1aio.a lib/
	install -D -t /usr/lib ./lib/libpru1aio.a

lib/libpru1aio.so.1.0.0: pru1aio.o
	install -D -t /usr/include ./include/pru1aio.h
	$(CC) $(CFLAGS) -c -fPIC pru1aio.c -o pru1aio.fo
	$(CC) -shared $(CFLAGS),-soname,libpru1aio.so.1 -o libpru1aio.so.1.0.0 pru1aio.fo
	rm pru1aio.fo
	mv libpru1aio.so.1.0.0 lib
	cp lib/libpru1aio.so.1.0.0 lib/libpru1aio.so
	install -D -t /usr/lib ./lib/libpru1aio.so
	ldconfig

static: $(OBJ) libpru1aio.a
	$(CC)  -std=c99 -static main.c -L./ -lpru1aio -lpthread -lprussdrv -o pru1aio
	
pru1aio: $(OBJ) main.c
	$(CC) $(CFLAGS) main.c $(INLDLIBS) -o pru1aio