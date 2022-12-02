#.PHONY: all clean install load uninstall
.PHONY: all clean install uninstall

# Choose which module to build
MODULE_NAME ?= rexgen_usb

KERNEL_PATH ?= /lib/modules/`uname -r`
KDIR ?= $(KERNEL_PATH)/build

REXGEN_SRC_DIR = `pwd`/src

$(info Selected module $(MODULE_NAME))

all:
#	@echo $(REXGEN_SRC_DIR)
	make -s -C $(KDIR)  M=$(REXGEN_SRC_DIR) modules

clean:
	make -C $(KDIR) M=$(REXGEN_SRC_DIR) clean

install:
	make -C $(KDIR) M=$(REXGEN_SRC_DIR) modules_install
	depmod -a

load:
	modprobe $(MODULE_NAME)

uninstall:
	rm $(addprefix $(KERNEL_PATH)/extra/$(MODULE_NAME), .ko .ko.gz .ko.xz) 2>/dev/null || true
	rmmod $(MODULE_NAME) 2>/dev/null || true
	depmod -a
