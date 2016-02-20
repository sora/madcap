KERNELSRCDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(shell pwd)
VERBOSE = 0


c_flags = -O0 -Wall

CC = gcc $(c_flags)

obj-m := raven.o


all:
	make -C $(KERNELSRCDIR) M=$(BUILD_DIR) V=$(VERBOSE) modules

.c.o:
	$(CC) -c $< -o $@

clean:
	make -C $(KERNELSRCDIR) M=$(BUILD_DIR) clean
