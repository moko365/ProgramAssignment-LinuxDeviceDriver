#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/uaccess.h>

struct cdata_dev {
};

int cdata_device_register(struct cdata_dev *ops)
{
	if (ops == NULL)
	    goto fail1;

	return 0;

fail1:
	printk(KERN_ALERT "cdata_dev: register failed.\n");
	return -1;
}

int cdata_device_unregister(struct cdata_dev *ops)
{
}

/***************************** sysfs ****************************/

static ssize_t cdata_show_version(struct class *cls, char *buf)
{
	return sprintf(buf, "CDATA CLASS V1.0\n");
}

static ssize_t cdata_handle_connect(struct class *cls, const char *buf,
					size_t count)
{
	return 0;
}

static CLASS_ATTR(cdata, 0666, cdata_show_version, cdata_handle_connect);

static struct class *cdata_class;

static int __init cdata_dev_init(void)
{
	cdata_class = class_create(THIS_MODULE, "cdata_android");

	class_create_file(cdata_class, &class_attr_cdata);
}

static void __exit cdata_dev_exit(void)
{
	class_remove_file(cdata_class, &class_attr_cdata);

	class_destroy(cdata_class);
}

module_init(cdata_dev_init);
module_exit(cdata_dev_exit);

EXPORT_SYMBOL(cdata_device_register);
EXPORT_SYMBOL(cdata_device_unregister);

MODULE_LICENSE("GPL");
