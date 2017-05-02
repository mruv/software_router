#!/bin/bash

#this is a make file

CC=gcc
CFLAGS=-g -Wall -Wextra

all:run

run:server.c server.h
	$(CC) $(CFLAGS) server.c -o run

clean:
	rm -rf run
