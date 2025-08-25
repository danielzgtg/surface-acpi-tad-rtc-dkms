# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for the driver
#

sources := dkms.conf
sources += Makefile
sources += src/Kconfig
sources += src/Makefile
sources += src/surface-acpi-tad-rtc.c

KVERSION ?= $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build

DEBUG ?= y

all:
	$(MAKE) -C $(KDIR) M=$(PWD)/src CONFIG_RTC_DRV_SURFACE=m modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/src CONFIG_RTC_DRV_SURFACE=m clean

check:
	$(KDIR)/scripts/checkpatch.pl -f -q --no-tree --strict --ignore EMBEDDED_FILENAME,UNCOMMENTED_DEFINITION $(sources)

dkms-install: $(sources)
	./dkms.sh install

dkms-uninstall:
	./dkms.sh uninstall
