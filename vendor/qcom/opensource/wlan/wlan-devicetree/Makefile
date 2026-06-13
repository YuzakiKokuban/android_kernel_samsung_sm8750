# SPDX-License-Identifier: GPL-2.0
KBUILD_OPTIONS += KBUILD_EXTMOD_DTS=.

all: dtbs

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean

%:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $@ $(KBUILD_OPTIONS)
