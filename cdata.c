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
#include <asm/io.h>
#include <asm/uaccess.h>

#define CDATA_MAJOR 121

static int cdata_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "cdata in open: filp = %p\n", filp);

	return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "cdata in close");
	return 0;
}

static int cdata_ioctl(struct inode *inode, struct file *filp, 
			unsigned int cmd, unsigned long arg)
{
}

static struct file_operations cdata_fops = {
    owner:      THIS_MODULE,

    // System call implementation
	open:		cdata_open,
    release:    cdata_close,
    ioctl:      cdata_ioctl,
};

int cdata_init_module(void)
{
	if (register_chrdev(CDATA_MAJOR, "cdata", &cdata_fops)) {
	    printk(KERN_ALERT "cdata module: can't registered.\n");
    }
}

void cdata_cleanup_module(void)
{
	unregister_chrdev(CDATA_MAJOR, "cdata");
	printk(KERN_ALERT "cdata module: unregisterd.\n");
}

module_init(cdata_init_module);
module_exit(cdata_cleanup_module);

MODULE_LICENSE("GPL");
