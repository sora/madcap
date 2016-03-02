KERNELSRCDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(shell pwd)
VERBOSE = 0

OVBENCH?=no
flag_ovbench_yes = -DOVBENCH
flag_ovbench_no =

obj-y := madcap/ raven/ \
	protocol-drivers-3.19.0/gre/	\
	protocol-drivers-3.19.0/ipip/	\
	protocol-drivers-3.19.0/vxlan/	\
	protocol-drivers-3.19.0/nsh/	\
	netdevgen/

subdir-ccflags-y := -I$(src)/include $(flag_ovbench_$(OVBENCH))


all:
	make -C $(KERNELSRCDIR) M=$(BUILD_DIR) V=$(VERBOSE) modules

clean:
	make -C $(KERNELSRCDIR) M=$(BUILD_DIR) clean
