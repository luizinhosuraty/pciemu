# Makefile for tests
#
# Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
#
# SPDX-License-Identifier: GPL-2.0

targets := all clean test

subdirs := $(wildcard */)

$(targets): $(subdirs)
$(subdirs):
	@printf -- "--- Running 'make $(MAKECMDGOALS)' inside dir $@ ---\n"
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(targets) $(subdirs)

