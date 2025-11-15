# Common definitions
CC = gcc

# Compiler flags, paths and libraries
CFLAGS = -std=c11 -pedantic -Wall -Wextra -O3 -ggdb -gdwarf-4
LFLAGS = $(CFLAGS)
LIBS   = -lm -lpthread

TGTS = partdiff-posix
OBJS = partdiff.o askparams.o

# Targets ...
all: $(TGTS)

partdiff-posix: $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

partdiff.o: partdiff.c partdiff.h Makefile
	$(CC) -c $(CFLAGS) -o $@ partdiff.c

askparams.o: askparams.c partdiff.h Makefile
	$(CC) -c $(CFLAGS) -o $@ askparams.c

clean:
	$(RM) $(OBJS)
	$(RM) $(TGTS)
