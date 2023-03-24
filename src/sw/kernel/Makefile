# Makefile for the kernel module
#
# Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
#
# SPDX-License-Identifier: GPL-2.0
#

obj-m += pciemu.o
pciemu-objs += pciemu_module.o pciemu_dma.o pciemu_irq.o
ccflags-y=-I/hostdir/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
