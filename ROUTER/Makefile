#!/bin/bash

CC=gcc
CFLAGS=-g -Wall -Wextra


all:run

run:router.h router.c
	$(CC) $(CFLAGS) -o run router.c

clean:
	rm -rf run