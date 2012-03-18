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
#include "cdata_dev_class.h"

/* private data structure */
struct cdata_dev_data {
	struct cdata_dev 	*ops;
	struct miscdevice	misc;

	int 			minor;
};

static struct cdata_dev_data *cdata_dev_data[MAX_MINOR];

/************************ misc ******************************/

static int cdata_dev_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int cdata_dev_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static int cdata_dev_ioctl(struct inode *inode, struct file *filp, 
				unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations cdata_dev_fops = {
	owner:		THIS_MODULE,
	open:		cdata_dev_open,
	release:	cdata_dev_close,
	ioctl:		cdata_dev_ioctl,
};

/******************************************************/

int cdata_device_register(struct cdata_dev *ops)
{
	int ret;
	int minor;
	struct cdata_dev_data *data;
	char name[16];

	ret = -1;

	if (ops == NULL)
	    goto fail1;

	minor = ops->minor;
	if (minor > MAX_MINOR)
	    goto fail1;

	data = cdata_dev_data[minor];
	if (data != NULL)
	    goto fail1;

	data = (struct cdata_dev_data *)
	            kmalloc(sizeof(struct cdata_dev_data), GFP_KERNEL);
	memset(data, 0, sizeof(struct cdata_dev_data));

	data->ops = ops;
	data->minor = minor;
	cdata_dev_data[minor] = data;

	sprintf(name, "cdata%d\n", minor);
	data->misc.minor = 20+minor; 
	data->misc.name = name;
	data->misc.fops = &cdata_dev_fops;

	misc_register(&data->misc);
	printk(KERN_ALERT "cdata_dev: registering %s to misc\n", name);

	return 0;

fail1:
	printk(KERN_ALERT "cdata_dev: register failed.\n");
	return ret;
}

int cdata_device_unregister(struct cdata_dev *ops)
{
	int minor;
	struct cdata_dev_data *data;

	minor = ops->minor;
	data = cdata_dev_data[minor];

	misc_deregister(&data->misc);
	
	kfree(data);

	return 0;
}

/***************************** sysfs ****************************/

static ssize_t cdata_show_version(struct class *cls, char *buf)
{
	return sprintf(buf, "CDATA CLASS V1.0\n");
}

static ssize_t cdata_handle_connect(struct class *cls, const char *buf,
					size_t count)
{
	struct cdata_dev_data *data;
	struct cdata_dev *ops;
	char str[1];
	int v;
	int i;

	memcpy(str, buf, 1);

	/* atoi */
	v = str[0];
	v = v - '0';
	
	if ((v != 0) || (v != 1))
	    return 0;

	printk(KERN_ALERT "cdata_dev: connect_enable = %d\n", v);

	/** callback connect */
	for (i = 0; i < MAX_MINOR; i++) {
	    data = cdata_dev_data[i];

	    if (data == NULL)
		continue;

	    ops = data->ops;
	    if (v == 0) {
	        if (ops && ops->disconnect) 
		    ops->disconnect(ops);
	    } else {
	        if (ops && ops->connect) 
		    ops->connect(ops);
	    }
	}

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
