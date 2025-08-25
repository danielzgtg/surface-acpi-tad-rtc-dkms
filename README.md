# surface-acpi-tad-rtc-dkms

This is a driver that ensures a Surface Pro 7 running linux-surface boots with the correct time.

The original `rtc-surface.c` was suddenly removed from the linux-surface kernel, causing time problems https://github.com/linux-surface/linux-surface/issues/415, https://github.com/linux-surface/linux-surface/issues/1497, and https://github.com/linux-surface/linux-surface/issues/1658. This completely breaks internet access for me because systemd-timesyncd can't resolve ntp.ubuntu.com if DNSSEC is enabled in systemd-resolved and the date is wrong in a chicken-and-egg problem. When it was reintroduced, it didn't work.

### Building (in-tree)
* Copy the folder `src` to `drivers/rtc`
* Merge our `Makefile` into `drivers/rtc/Kconfig`
* Merge our `Kconfig` into `drivers/rtc/Makefile`
* Enable the driver by setting `RTC_DRV_SURFACE=m` in the kernel config

### Building (out-of-tree)
* Run `make`
* Run `sudo insmod src/surface-acpi-tad-rtc.ko`

### Building (DKMS)
* Run `sudo make dkms-install`
* Reboot
