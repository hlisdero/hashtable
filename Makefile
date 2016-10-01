CFLAGS=-g -std=c99 -Wall -Wconversion -Wno-sign-conversion -Werror
OBJ=pruebas_catedra.c main.c hash.c hash.h lista.c lista.h testing.c testing.h
CC=gcc
EXEC=pruebas

all:
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC)

valgrind:
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC)
	valgrind --leak-check=full --track-origins=yes --show-reachable=yes ./pruebas
