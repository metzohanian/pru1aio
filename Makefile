CC = gcc
CFLAGS+=-Wall -Werror -std=c99
LDLIBS+= -lpthread -lprussdrv
DEPS = pru1aio.h pru1aio.p pru1aio.hp
OBJ = main.o pru1aio.o

all: pru1aio.bin pru1aio clearpru.bin firmware

clean:
	rm -f pru1aio *.o *.bin *.csv
	rm firmware/*.dtbo
	rm /lib/firmware/*.dtbo

clearpru.bin: clearpru.p
	pasm -b $^
	
pru1aio.bin: pru1aio.p
	pasm -b $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

pru1aio: $(OBJ)
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $^
	
FW_DIR = firmware
.PHONY: firmware

firmware:
	$(MAKE) -C $(FW_DIR)