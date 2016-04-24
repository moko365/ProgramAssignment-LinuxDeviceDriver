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
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "cdata_ioctl.h"

#define CDATA_MAJOR 121
#define	BUF_SIZE 32

struct cdata_t {
	char *buf;
	unsigned int idx;
	wait_queue_head_t writeable;
	struct work_struct work;
};

void flush_data(struct work_struct *work);

void flush_data(struct work_struct *work)
{
    	struct cdata_t *cdata = container_of(work, struct cdata_t, work);
	char *buf = cdata->buf;
	int idx = cdata->idx;

	buf[idx] = '\0';

	printk(KERN_ALERT "buf = %s\n", buf);

    	cdata->idx = 0;
    	wake_up(&cdata->writeable);
}

static int cdata_open(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata;

	cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
	cdata->buf = (char *)kmalloc(BUF_SIZE, GFP_KERNEL);
	cdata->idx = 0;
	init_waitqueue_head(&cdata->writeable);
	INIT_WORK(&cdata->work, flush_data);

	filp->private_data = (void *)cdata;

	printk(KERN_ALERT "cdata in open: filp = %p\n", filp);

	return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	char *buf;
	unsigned int idx;

	buf = cdata->buf;
	idx = cdata->idx;

	buf[idx] = '\0';

	printk(KERN_ALERT "cdata in close: buf = %s\n", buf);

	kfree(cdata->buf);
	kfree(cdata);

	return 0;
}

static int cdata_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	char *buf;
	unsigned int idx;

	buf = cdata->buf;
	idx = cdata->idx;
	
	switch (cmd) {
	case IOCTL_EMPTY:
		printk(KERN_ALERT "in ioctl: IOCTL_EMPTY\n");
		idx = 0;
	    	return 0;
	case IOCTL_SYNC:
	    	printk(KERN_ALERT "in ioctl: IOCTL_SYNC\n");
	    	return 0;
	default:
		return -ENOTTY;
	}

	return 0;
}

static ssize_t cdata_write(struct file *filp, const char __user *user, 
	size_t len, loff_t *off)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	int idx;
	int i;

	for (i = 0; i < len; i++) {
		if (cdata->idx >= BUF_SIZE)
			schedule_work(&cdata->work);

		wait_event_interruptible(cdata->writeable, 
			!(cdata->idx >= BUF_SIZE));
		idx = cdata->idx;

        	copy_from_user(&cdata->buf[idx], &user[i], 1);

		idx++;
        	cdata->idx = idx;
    	}

	return len;
}

static const struct file_operations cdata_fops = {
    	owner:	THIS_MODULE,
    	open:	cdata_open,
    	release:	cdata_close,
	write:	cdata_write,
    	unlocked_ioctl:	cdata_ioctl
};

static struct miscdevice cdata_miscdev = {
	.minor	= 199,
	.name	= "cdata-misc",
	.fops	= &cdata_fops,
};

static int cdata_plat_probe(struct platform_device *pdev)
{
	int ret;

	ret = misc_register(&cdata_miscdev);
	if (ret < 0) {
		printk(KERN_ALERT "misc_register failed\n");
		goto exit;
	}

	return 0;
exit:
	return -1;
}

static int cdata_plat_remove(struct platform_device *pdev)
{
    misc_deregister(&cdata_miscdev);
    return 0;
}

static struct platform_driver cdata_plat_driver = {
	.driver = {
		   .name	= "cdata",
		   .owner	= THIS_MODULE
		   },
	.probe = cdata_plat_probe,
	.remove = cdata_plat_remove,

};

static int __init cdata_init_module(void)
{
	platform_driver_register(&cdata_plat_driver);
	printk(KERN_ALERT "cdata module: registerd.\n");
}

void cdata_cleanup_module(void)
{
	platform_driver_unregister(&cdata_plat_driver);
	printk(KERN_ALERT "cdata module: unregisterd.\n");
}

module_init(cdata_init_module);
module_exit(cdata_cleanup_module);

MODULE_LICENSE("GPL");
