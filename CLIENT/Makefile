#!/bin/bash

CC=gcc
CFLAGS=-g -Wall -Wextra


all:run

run:client.h client.c
	$(CC) $(CFLAGS) -o run client.c


clean:
	rm -rf run