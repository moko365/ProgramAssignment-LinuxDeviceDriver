#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "cdata_ioctl.h"

#ifdef CONFIG_SMP
#define __SMP__
#endif

#define	CDATA_MAJOR 121 

struct cdata_t {
    char    *buf;
    int     index;
    wait_queue_head_t	wq;
    struct timer_list   timer;
};

void flush_buffer(unsigned long priv)
{
    struct cdata_t *cdata = (struct cdata_t *)priv;

    cdata->index = 0;

    wake_up(&cdata->wq);
}

static int cdata_open(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata;
    
    cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
    cdata->buf = (char *)kmalloc(64, GFP_KERNEL);

    cdata->index = 0;
    init_waitqueue_head (&cdata->wq);

    init_timer(&cdata->timer);

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
    struct timer_list *timer = &cdata->timer;
    int i;

    for (i = 0; i < size; i++) {
        copy_from_user(&cdata->buf[cdata->index], &buf[i], 1);
        cdata->index++;

        if (cdata->index >= 64) {
            printk(KERN_ALERT "cdata: buffer full\n");
            // schedule execution (deferred execution)
            timer->expires = jiffies + 5*HZ;
            timer->function = flush_buffer;
            timer->data = (unsigned long)cdata;

            add_timer(timer);

            interruptible_sleep_on(&cdata->wq);
            
            // Ok, buffer is empty
            printk(KERN_ALERT "cdata: buffer empty\n");
        }
    }

	return 0;
}

static int cdata_release(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;

	printk(KERN_ALERT "cdata: in cdata_release()\n");
	printk(KERN_ALERT "cdata: flush buf = %s\n", cdata->buf);
	return 0;
}

struct file_operations cdata_fops = {	
    owner:      THIS_MODULE,
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
