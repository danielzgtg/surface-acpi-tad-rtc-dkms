// SPDX-License-Identifier: GPL-2.0+

#include <linux/platform_device.h>
#include <linux/rtc.h>

static inline bool surface_acpi_tad_rtc_valid_tm(const struct rtc_time *tm) {
	return !(tm->tm_sec < 0 || tm->tm_sec > 59 ||
		tm->tm_min < 0 || tm->tm_min > 59 ||
		tm->tm_hour < 0 || tm->tm_hour > 23 ||
		tm->tm_mday < 1 || tm->tm_mday > 31 ||
		tm->tm_mon < 0 || tm->tm_mon > 11 ||
		tm->tm_year < 0 || tm->tm_year > 9999 - 1900 ||
		tm->tm_isdst < 0 || tm->tm_isdst > 1);
}

static struct device *acpi_tad_device;
static struct device_attribute *dev_attr_time_dev_attr;

static int surface_acpi_tad_rtc_read_time(struct device *, struct rtc_time *tm)
{
	char acpi_buf_time_show_buf[sizeof("9999:12:31:23:59:59:-1440:3\n")];
	const ssize_t acpi_buf_time_show_buf_count =
		dev_attr_time_dev_attr->show(acpi_tad_device, dev_attr_time_dev_attr, acpi_buf_time_show_buf);
	if (acpi_buf_time_show_buf_count < 0) {
		return (int) acpi_buf_time_show_buf_count;
	}
	BUG_ON(acpi_buf_time_show_buf_count >= sizeof(acpi_buf_time_show_buf));
	if (WARN_ON(acpi_buf_time_show_buf_count < sizeof("1900:1:1:0:0:0:0:0\n") - 1) ||
		WARN_ON(acpi_buf_time_show_buf[acpi_buf_time_show_buf_count] != '\0') ||
		WARN_ON(acpi_buf_time_show_buf[acpi_buf_time_show_buf_count-1] != '\n')) {
		return -EUCLEAN;
	}
	pr_warn("surface_acpi_tad_rtc_read_time: got %s", acpi_buf_time_show_buf);
	int tz = 0;
	unsigned daylight = 0;
	if (WARN_ON(sscanf(acpi_buf_time_show_buf, "%u:%u:%u:%u:%u:%u:%d:%u\n", // NOLINT(*-err34-c)
		               &tm->tm_year, &tm->tm_mon, &tm->tm_mday, &tm->tm_hour, &tm->tm_min, &tm->tm_sec,
		               &tz, &daylight) != 8)) {
		return -EUCLEAN;
	}
	tm->tm_mon -= 1;
	tm->tm_year -= 1900;
	WARN_ON(tz != 2047);
	pr_warn("surface_acpi_tad_rtc_read_time: before %d:%d:%d --- %d:%d:%d --- %d:%d:%d\n",
			tm->tm_sec, tm->tm_min, tm->tm_hour,
			tm->tm_mday, tm->tm_mon, tm->tm_year,
			tm->tm_wday, tm->tm_yday, tm->tm_isdst);
	if (!surface_acpi_tad_rtc_valid_tm(tm)) {
		return -ERANGE;
	}
	rtc_time64_to_tm(rtc_tm_to_time64(tm), tm); // tm_wday, tm_yday
	pr_warn("surface_acpi_tad_rtc_read_time: after %d:%d:%d --- %d:%d:%d --- %d:%d:%d\n",
			tm->tm_sec, tm->tm_min, tm->tm_hour,
			tm->tm_mday, tm->tm_mon, tm->tm_year,
			tm->tm_wday, tm->tm_yday, tm->tm_isdst);
	tm->tm_isdst = !!daylight;
	return 0;
}

static int surface_acpi_tad_rtc_set_time(struct device *, struct rtc_time *tm)
{
	pr_warn("surface_acpi_tad_rtc_set_time: before %d:%d:%d --- %d:%d:%d --- %d:%d:%d\n",
			tm->tm_sec, tm->tm_min, tm->tm_hour,
			tm->tm_mday, tm->tm_mon, tm->tm_year,
			tm->tm_wday, tm->tm_yday, tm->tm_isdst);
	struct rtc_time tm_check;
	BUG_ON(tm->tm_sec == 60);
	rtc_time64_to_tm(rtc_tm_to_time64(tm), &tm_check);
	WARN_ON(memcmp(tm, &tm_check, sizeof(tm_check)));
	tm = &tm_check;
	pr_warn("surface_acpi_tad_rtc_set_time: after %d:%d:%d --- %d:%d:%d --- %d:%d:%d\n",
			tm->tm_sec, tm->tm_min, tm->tm_hour,
			tm->tm_mday, tm->tm_mon, tm->tm_year,
			tm->tm_wday, tm->tm_yday, tm->tm_isdst);
	if (!surface_acpi_tad_rtc_valid_tm(tm)) {
		return -ERANGE;
	}
	char acpi_buf_time_store_buf[sizeof("9999:12:31:23:59:59:0:1\n")];
	const ssize_t acpi_buf_time_store_buf_count = sprintf(acpi_buf_time_store_buf, "%d:%d:%d:%d:%d:%d:0:%d\n",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,  tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_isdst);
	BUG_ON(acpi_buf_time_store_buf_count >= sizeof(acpi_buf_time_store_buf));
	BUG_ON(acpi_buf_time_store_buf_count < sizeof("1900:1:1:0:0:0:0:0\n") - 1);
	pr_warn("surface_acpi_tad_rtc_set_time sending %s", acpi_buf_time_store_buf);
	const ssize_t ret = dev_attr_time_dev_attr->store(
		acpi_tad_device, dev_attr_time_dev_attr, acpi_buf_time_store_buf, acpi_buf_time_store_buf_count);
	if (ret < 0) {
		return (int) ret;
	}
	if (WARN_ON(ret != acpi_buf_time_store_buf_count)) {
		return -EIO;
	}
	return 0;
}

static const struct rtc_class_ops surface_rtc_ops = {
	.read_time = surface_acpi_tad_rtc_read_time,
	.set_time = surface_acpi_tad_rtc_set_time,
};

static void surface_acpi_tad_rtc_pdev_release(struct device *) {
	// Nothing to do
}

static struct platform_device pdev = {
	.name = "surfaceacpitadrtc",
	.dev = {
		.release = surface_acpi_tad_rtc_pdev_release,
	},
};

static struct rtc_device *rtc;

static int surface_acpi_tad_rtc_init(void) {
	acpi_tad_device = bus_find_device_by_name(&platform_bus_type, NULL, "ACPI000E:00");
	if (!acpi_tad_device) {
		return -ENODEV;
	}
	int ret = 0;
	if (!acpi_tad_device->driver || !try_module_get(acpi_tad_device->driver->owner)) {
		ret = -ENODEV;
		goto fail3;
	}
	BUG_ON(!acpi_tad_device->kobj.ktype);
	BUG_ON(!acpi_tad_device->kobj.ktype->sysfs_ops);
	BUG_ON(!acpi_tad_device->kobj.sd);
	struct kernfs_node * dev_attr_time_attr_kernfs = sysfs_get_dirent(acpi_tad_device->kobj.sd, "time");
	if (!dev_attr_time_attr_kernfs) {
		ret = -ENOTTY;
		goto fail2;
	}
	struct attribute *dev_attr_time_attr = dev_attr_time_attr_kernfs->priv;
	BUG_ON(!dev_attr_time_attr);
	BUG_ON(strcmp(dev_attr_time_attr->name, "time"));
	BUG_ON(dev_attr_time_attr->mode != (S_IRUGO|S_IWUSR));
	dev_attr_time_dev_attr = container_of(dev_attr_time_attr, struct device_attribute, attr);
	BUG_ON(!dev_attr_time_dev_attr->show);
	BUG_ON(!dev_attr_time_dev_attr->store);
	ret = platform_device_register(&pdev);
	if (ret)
		goto fail2;
	rtc = devm_rtc_allocate_device(&pdev.dev);
	if (IS_ERR(rtc)) {
		ret = (int) PTR_ERR(rtc);
		goto fail1;
	}
	rtc->ops = &surface_rtc_ops;
	rtc->range_min = RTC_TIMESTAMP_BEGIN_1900;
	rtc->range_max = RTC_TIMESTAMP_END_9999;
	ret = devm_rtc_register_device(rtc);
	if (ret)
		goto fail1;
	return 0;

fail1:
	platform_device_unregister(&pdev);
fail2:
	module_put(acpi_tad_device->driver->owner);
fail3:
	put_device(acpi_tad_device);
	return ret;
}

static void surface_acpi_tad_rtc_exit(void) {
	platform_device_unregister(&pdev);
	module_put(acpi_tad_device->driver->owner);
	put_device(acpi_tad_device);
}

module_init(surface_acpi_tad_rtc_init);
module_exit(surface_acpi_tad_rtc_exit);
MODULE_AUTHOR("Daniel Tang <danielzgtg.opensource@gmail.com>");
MODULE_DESCRIPTION("Surface Pro 7 acpi_tad to /dev/rtc forwarding driver");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: acpi_tad");
