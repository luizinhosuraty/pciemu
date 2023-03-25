# Makefile for userspace programs
#
# Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
#
# SPDX-License-Identifier: GPL-2.0
#

KBLUE := "\e[1;36m"
KNORM := "\e[0m"

build_dir := build/
include_dir := $(abspath ../../../include/)
includes := $(addprefix -I, $(include_dir))
cflags := -Wall -Werror -O2 $(includes)
targets := pciemu_example

.PHONY : all
all: $(targets)

$(build_dir)%.o : %.c Makefile | $(build_dir)
	@printf $(KBLUE)"---- building $@ ----\n"$(KNORM)
	$(CC) $(cflags) -c -MMD -MP $< -o $@

$(targets): %: $(build_dir)%.o
	@printf $(KBLUE)"---- linking $@----\n"$(KNORM)
	$(CC) -o $@ $<

$(build_dir):
	@printf $(KBLUE)"---- create $@ dir ----\n"$(KNORM)
	mkdir -p $(build_dir)

.PHONY : clean
clean:
	@printf $(KBLUE)"---- cleaning ----\n"$(KNORM)
	rm -rf $(targets)
	rm -rf $(build_dir)
