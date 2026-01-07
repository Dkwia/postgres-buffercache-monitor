CC = gcc
PG_INC = /home/bo/opt/pgsql/include/server
PG_LIB = /home/bo/opt/pgsql/lib

CFLAGS = -fPIC -shared -O2 -Wall -Wextra -I$(PG_INC)
LDFLAGS = -L$(PG_LIB)

all:
	$(CC) $(CFLAGS) my_buffer_dump.c -o my_buffer_dump.so $(LDFLAGS)

install:
	cp my_buffer_dump.so $(PG_LIB)

