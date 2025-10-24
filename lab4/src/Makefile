CC=gcc
CFLAGS=-Wall -Wextra -O2 -I.

# Собирает все программы лабораторной работы 4
all: parallel_min_max zombie_demo process_memory parallel_sum

parallel_min_max: utils.o find_min_max.o parallel_min_max.c
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

zombie_demo: zombie_demo.c
	$(CC) -o zombie_demo zombie_demo.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

parallel_sum: utils.o sum_lib.o parallel_sum.c
	$(CC) -o parallel_sum utils.o sum_lib.o parallel_sum.c $(CFLAGS) -lpthread

sum_lib.o: sum_lib.c sum_lib.h
	$(CC) -o sum_lib.o -c sum_lib.c $(CFLAGS)

clean:
	rm -f utils.o find_min_max.o sum_lib.o parallel_min_max zombie_demo process_memory parallel_sum

.PHONY: all clean