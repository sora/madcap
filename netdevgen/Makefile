KERNELSRCDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(shell pwd)
VERBOSE = 0

OVBENCH?=no
flag_ovbench_yes = -DOVBENCH
flag_ovbench_no =

obj-m := netdevgen.o

ccflags-y := $(flag_ovbench_$(OVBENCH))

all:
	make -C $(KERNELSRCDIR) M=$(BUILD_DIR) V=$(VERBOSE) modules

clean:
	make -C $(KERNELSRCDIR) M=$(BUILD_DIR) clean
