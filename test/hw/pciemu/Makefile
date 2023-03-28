# Makefile for unit test of pciemu hw functions
#
# Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
#
# SPDX-License-Identifier: GPL-2.0
#

directories := $(shell git rev-parse --show-toplevel)/makefiles/directories.mk
include $(directories)

cflags += `pkg-config --cflags glib-2.0`

fakes_src := qemu.fake.c pciemu_dma.fake.c pciemu_irq.fake.c pciemu_mmio.fake.c

# for including the source files 
src_hw_pciemu_dir := $(src_dir)/hw/pciemu

includes += $(addprefix -I, $(qemu_include_dir)\
	    		    $(include_dir)/hw\
			    $(qemu_build_include_dir)\
			    $(src_hw_pciemu_dir))

targets := pciemu pciemu_dma pciemu_irq pciemu_mmio

unittest := $(root_dir)/makefiles/unittest.mk
include $(unittest)
