#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <asm/io.h>
#include "cdata_ioctl.h"

#ifdef CONFIG_SMP
#define __SMP__
#endif

#define	CDATA_MAJOR 121 

wait_queue_head_t	rq;

struct cdata_t {
    char    *buf;
    int     index;
};

static int cdata_open(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata;
    
    cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
    cdata->buf = (char *)kmalloc(64, GFP_KERNEL);

    filp->private_data = (void *)cdata;

	return 0;
}

static int cdata_ioctl(struct inode *inode, struct file *filp, 
			unsigned int cmd, unsigned long arg)
{
	printk(KERN_ALERT "cdata: in cdata_ioctl()\n");
}

static ssize_t cdata_read(struct file *filp, char *buf, 
				size_t size, loff_t *off)
{
	printk(KERN_ALERT "cdata: in cdata_read()\n");
}

static ssize_t cdata_write(struct file *filp, const char *buf, 
				size_t size, loff_t *off)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;

    copy_from_user(buf, cdata->buf, size);

	return 0;
}

static int cdata_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "cdata: in cdata_release()\n");
	return 0;
}

struct file_operations cdata_fops = {	
	open:		cdata_open,
	release:	cdata_release,
	ioctl:		cdata_ioctl,
	read:		cdata_read,
	write:		cdata_write,
};

static struct miscdevice cdata_misc = {
	minor:	20,
	name:	"cdata-test",
	fops:	&cdata_fops,
};

int my_init_module(void)
{
	if (misc_register(&cdata_misc)) {
		printk(KERN_ALERT "cdata: register failed\n");
		return -1;
	}

	printk(KERN_ALERT "cdata module: registered.\n");

	return 0;
}

void my_cleanup_module(void)
{
	misc_deregister(&cdata_misc);
	printk(KERN_ALERT "cdata module: unregisterd.\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);

MODULE_LICENSE("GPL");
