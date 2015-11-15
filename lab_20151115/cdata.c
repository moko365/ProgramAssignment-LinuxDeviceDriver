/**
 * Filename: cdata.c
 * 
 * Linux device driver training (2015.11.15)
 */
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
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "cdata_ioctl.h"

#define CDATA_MINOR	199
#define BUF_SIZE	128

struct cdata_t {
    unsigned char *data;
    unsigned int index;
    wait_queue_head_t wq;
    struct work_struct work;
};

void flush_data(struct work_struct *work);

static long cdata_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch(cmd) {
    case IOCTL_EMPTY:
	printk(KERN_ALERT "in ioctl: IOCTL_EMPTY\n");
	break;
    default:
	return -ENOTTY;
    }

    printk(KERN_ALERT "cdata: in cdata_ioctl\n");
    return 0;
}

static int cdata_open(struct inode *inode, struct file *filp) 
{
    struct cdata_t *cdata;

    cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);

    cdata->data = (unsigned char *)kzalloc(BUF_SIZE, GFP_KERNEL);
    cdata->index = 0;
    init_waitqueue_head(&cdata->wq);
    INIT_WORK(&cdata->work, flush_data);

    filp->private_data = (void *)cdata;

    return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;

    kfree(cdata->data);
    kfree(cdata);

    return 0;
}

void flush_data(struct work_struct *work)
{
    struct cdata_t *cdata = container_of(work, struct cdata_t, work);

    cdata->index = 0;
    wake_up(&cdata->wq);
}

static ssize_t cdata_write(struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
    char *data = cdata->data;
    int index = cdata->index;
    DEFINE_WAIT(wait);
    int i;

    for (i = 0; i < size; i++) {
        copy_from_user(&data[index], &buf[i], 1);
        index++;

        if (index >= BUF_SIZE) {
	    printk(KERN_ALERT "cdata: buffer full\n");
#if 1
repeat:
	    prepare_to_wait(&cdata->wq, &wait, TASK_INTERRUPTIBLE);
	    schedule_work(&cdata->work);
	    schedule();

	    index = cdata->index; // *sync*
	    if (index) 
	        goto repeat;

	    finish_wait(&cdata->wq, &wait);
#else
	    wait_event_interruptible(cdata->wq, !cdata->index);
	    schedule_work(&cdata->work);
	    schedule();
	    index = cdata->index;
#endif
        }
    }
    
    cdata->index = index;

    return 0;
}

static struct file_operations cdata_fops = {	
    owner: THIS_MODULE,
    open: cdata_open,
    release: cdata_close,
    write: cdata_write,
    compat_ioctl: cdata_ioctl
};

static struct miscdevice cdata_misc = {
	minor:	CDATA_MINOR,
	name:	"cdata",
	fops:	&cdata_fops,
};

static int ldt_plat_probe(struct platform_device *pdev)
{
    if (misc_register(&cdata_misc) < 0) {
        printk(KERN_INFO "CDATA: can't register driver\n");
        return -1;
    }
}

static int ldt_plat_remove(struct platform_device *pdev)
{
    misc_deregister(&cdata_misc);
    return 0;
}

static struct platform_driver ldt_plat_driver = {
	.driver = {
		   .name	= "cdata_dummy",
		   .owner	= THIS_MODULE
		   },
	.probe = ldt_plat_probe,
	.remove = ldt_plat_remove,

};

int my_init_module(void)
{
    platform_driver_register(&ldt_plat_driver);
    printk(KERN_ALERT "cdata module: registered.\n");

    return 0;
}

void my_cleanup_module(void)
{
    platform_driver_unregister(&ldt_plat_driver);
    printk(KERN_ALERT "cdata module: deregistered.\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);

MODULE_LICENSE("GPL");
