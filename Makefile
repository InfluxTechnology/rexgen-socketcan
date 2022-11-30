
#.PHONY: all clean install load uninstall
.PHONY: all clean install uninstall

# Choose which module to build
MODULE_NAME ?= rexgen_usb
#KV_MODULE_NAME ?= kvaser_pciefd

KERNEL_PATH ?= /lib/modules/`uname -r`
#KERNEL_PATH ?= /lib/modules/5.4.0-47-generic
KDIR ?= $(KERNEL_PATH)/build

KERNEL_CAN_DIR = kernel/drivers/net/can
REXGEN_SRC_DIR = `pwd`/$(KERNEL_CAN_DIR)
REXGEN_SRC_DIR := $(REXGEN_SRC_DIR)/usb/rexgen_usb

#ifeq ($(KV_MODULE_NAME), kvaser_usb)
#KV_CONFIG_FLAGS = CONFIG_CAN_KVASER_USB=m
#KVASER_SRC_DIR := $(KVASER_SRC_DIR)/usb/kvaser_usb
#else ifeq ($(KV_MODULE_NAME), kvaser_pciefd)
#KV_CONFIG_FLAGS = CONFIG_CAN_KVASER_PCIEFD=m
#else
#$(error Unknown KV_MODULE_NAME='$(KV_MODULE_NAME)')
#endif

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
