KERNELSRCDIR = /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(shell pwd)
VERBOSE = 0


c_flags = -O0 -Wall

CC = gcc $(c_flags)

obj-m := madcap.o


all:
	make -C $(KERNELSRCDIR) SUBDIRS=$(BUILD_DIR) KBUILD_VERBOSE=$(VERBOSE)  modules

.c.o:
	$(CC) -c $< -o $@

clean:
	rm -f *.o
	rm -f *.ko
	rm -f *.mod.c
	rm -f *~
	rm modules.order
	rm Module.symvers
