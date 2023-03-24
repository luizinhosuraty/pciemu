# unittest.mk - Makefile helper file for unit testing
#
# Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
#
# SPDX-License-Identifier: GPL-2.0
#

KBLUE := "\e[1;36m"
KNORM := "\e[0m"

fakes_obj := $(addprefix $(fakes_build_dir)/, $(fakes_src:.c=.o))
fakes_depfiles := $(addprefix $(fakes_build_dir)/, $(fakes_src:.c=.d))
target_depfiles := $(addsuffix .c, $(target))

includes += $(addprefix -I, $(include_dir)\
			    $(test_include_dir))

cflags += -Wall -Werror -O2 $(includes)

.PHONY : all
all: $(targets)

$(fakes_build_dir)/%.o : $(fakes_dir)/%.c Makefile | $(fakes_build_dir)
	@printf $(KBLUE)"---- building $@ ----\n"$(KNORM)
	$(CC) $(cflags) -c -MMD -MP $< -o $@

$(build_dir)/%.o : %.c Makefile | $(build_dir)
	@printf $(KBLUE)"---- building $< $@ ----\n"$(KNORM)
	$(CC) $(cflags) -c -MMD -MP $< -o $@

$(targets):: %: $(build_dir)/%.o $(fakes_obj)
	@printf $(KBLUE)"---- linking $@ ----\n"$(KNORM)
	$(CC) $(ldflags) -o $@ $(fakes_obj) $<

$(build_dir):
	@printf $(KBLUE)"---- create $@ dir ----\n"$(KNORM)
	mkdir -p $(build_dir)/fakes

.PHONY : clean
clean:
	@printf $(KBLUE)"---- cleaning ----\n"$(KNORM)
	rm -rf $(targets)
	rm -rf $(build_dir)

test: $(targets)
	@printf $(KBLUE)"---- running tests ----\n"$(KNORM)
	@for f in $^ ; do echo $$f; ./$$f ; r=$$((r+$$?)) ; done; exit $$r

-include $(fakes_depfiles)
-include $(target_depfiles)
