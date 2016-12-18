#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "cdata_ioctl.h"

#define CDATA_MAJOR 121

struct cdata_t {
	char *buf;
	unsigned int offset;
	wait_queue_head_t writeable;
};

static int cdata_open(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata;

	cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
	cdata->buf = (char *)kmalloc(64, GFP_KERNEL);
	cdata->offset = 0;
	init_waitqueue_head(&cdata->writeable);

	filp->private_data = (void *)cdata;

	printk(KERN_ALERT "cdata in open: filp = %p\n", filp);
	return 0;
}

static long cdata_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	char *buf = cdata->buf;
	unsigned int offset = cdata->offset;

	switch (cmd) {
	case IOCTL_EMPTY:
		printk(KERN_ALERT "in ioctl: IOCTL_EMPTY\n");
		break;
	case IOCTL_SYNC:
		printk(KERN_ALERT "in ioctl: IOCTL_SYNC\n");
		break;
	case IOCTL_NAME:
		break;
	default:
		return -ENOTTY;
	}
	return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	char *buf = cdata->buf;
	unsigned int offset = cdata->offset;

	buf[1] = '\0';
	printk(KERN_ALERT "buf = %s\n", buf);

	return 0;
}

static ssize_t cdata_write(struct file *filp, const char __user *user,
                size_t size, loff_t *off)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	char *buf = cdata->buf;
	unsigned int offset = cdata->offset;
	int i;

	for (i = 0; i < size; i++) {
		wait_event_interruptible(cdata->writeable, !(offset >= 64) );

		copy_from_user(&buf[offset++], &user[i], 1);
	}

	cdata->offset = offset;

	return 0;
}

static struct file_operations cdata_fops = {	
	owner:		THIS_MODULE,
	open:		cdata_open,
	release:	cdata_close,
	compat_ioctl:	cdata_ioctl,
};

int cdata_init_module(void)
{
	if (register_chrdev(CDATA_MAJOR, "cdata", &cdata_fops)) {
	    printk(KERN_ALERT "cdata module: can't registered.\n");
	}

	printk(KERN_ALERT "cdata module: registerd.\n");
	return 0;
}

void cdata_cleanup_module(void)
{
	unregister_chrdev(CDATA_MAJOR, "cdata");
	printk(KERN_ALERT "cdata module: unregisterd.\n");
}

module_init(cdata_init_module);
module_exit(cdata_cleanup_module);

MODULE_DESCRIPTION("CDATA - Linux Driver Template");
MODULE_AUTHOR("jollen <jollen@jollen.org>");
MODULE_LICENSE("GPL");
