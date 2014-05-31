all:  test_monkey_lua

_PATH     = $(patsubst $(monkey_root)/%, %, $(CURDIR))
INC_EXTRA = "./"
INCDIR    = ../../../src/include -I$(INC_EXTRA)
CC	= @echo "  CC   $(_PATH)/$@"; gcc
CFLAGS	=   -std=gnu99 -Wall -Wextra -g -rdynamic
LDFLAGS =  -llua -lcunit
OBJECTS = ../lua.o lua_test.o



.c.o:
	$(CC) $(CFLAGS) $(DEFS) -I$(INCDIR) -c $*.c -o $*.o


clean:
	rm -rf *.[od] *~ *.*so* *.o test_monkey_lua ../*.o



test_monkey_lua: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFS)  -o $@ $^ -lc
	./test_monkey_lua
