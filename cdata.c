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
#include "cdata_ioctl.h"

struct cdata_t {
    unsigned long *fb;
};

static int cdata_open(struct inode *inode, struct file *filp)
{
	int i;
	int minor;
	struct cdata_t *cdata;

	printk(KERN_INFO "CDATA: in open\n");

	minor = MINOR(inode->i_rdev);
	printk(KERN_INFO "CDATA: minor = %d\n", minor);

	cdata= kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
	cdata->fb = ioremap(0x33f00000, 320*240*4);

	return 0;
}

static ssize_t cdata_read(struct file *filp, char *buf, size_t size, loff_t *off)
{
}

static ssize_t cdata_write(struct file *filp, const char *buf, size_t size, 
			loff_t *off)
{
	unsigned int i;

	printk(KERN_INFO "CDATA: in write\n");
	printk(KERN_INFO "buf = %s\n", buf);

	return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static int cdata_ioctl(struct inode *inode, struct file *filp, 
		unsigned int cmd, unsigned long arg)
{
	int n;
	unsigned long *fb;
	int i;

	switch (cmd) {
	    case CDATA_CLEAR:
	        n = *((int *)arg); // FIXME: dirty
		printk(KERN_INFO "CDATA_CLEAR: %d pixel\n", n);

		// FIXME: dirty
		fb = ioremap(0x33f00000, n*4);
		for (i = 0; i < n; i++)
		    writel(0x00ff0000, fb++);

	        break;
	}
}

static struct file_operations cdata_fops = {	
	owner:		THIS_MODULE,
	open:		cdata_open,
	release:	cdata_close,
	read:		cdata_read,
	write:		cdata_write,
	ioctl:		cdata_ioctl,

};

int cdata_init_module(void)
{
	unsigned long *fb;
	int i;

	fb = ioremap(0x33f00000, 320*240*4);
	for (i = 0; i < 320*240; i++)
		writel(0x00ff0000, fb++);
	
	if (register_chrdev(121, "cdata", &cdata_fops) < 0) {
	    printk(KERN_INFO "CDATA: can't register driver\n");
	    return -1;
	}
	printk(KERN_INFO "CDATA: cdata_init_module\n");
	return 0;
}

void cdata_cleanup_module(void)
{
	unregister_chrdev(121, "cdata");
}

module_init(cdata_init_module);
module_exit(cdata_cleanup_module);

MODULE_LICENSE("GPL");
