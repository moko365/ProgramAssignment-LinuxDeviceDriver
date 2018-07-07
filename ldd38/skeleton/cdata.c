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
#include <linux/slab.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "cdata_ioctl.h"

#define CDATA_MAJOR 121

struct cdata_t {
	u8 *buf;
	unsigned int index;
	wait_queue_head_t writeable;
	struct timer_list timer;
};

static int cdata_open(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata;

	cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
	cdata->buf = (u8 *)kmalloc(16, GFP_KERNEL);
	cdata->index = 0;

	init_waitqueue_head(&cdata->writeable);
	init_timer(&cdata->timer);

	filp->private_data = cdata;

	return 0;
}

static int cdata_release(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	u8 *buf = cdata->buf;
	unsigned int index = cdata->index;

    	buf[index] = '\0';
	printk("cdata: buf = %s\n", buf);

	kfree(cdata->buf);
	kfree(cdata);

	return 0;
}

static long cdata_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	u8 *buf = cdata->buf;
	unsigned int index = cdata->index;

	switch(cmd) {
	case CDATA_EMPTY:
		printk(KERN_ALERT "cdata: CDATA_EMPTY\n");
		break;
	case CDATA_SYNC:
		buf[index] = '\0';
		printk(KERN_ALERT "cdata: buf = %s\n", buf);
		break;
	case CDATA_NAME:
		if (index < 15)
			buf[index++] = (u8)arg;
		break;
	default:
		return -ENOTTY;
	}

	cdata->index = index;

	return 0;
}

void write_data(unsigned long arg)
{
	struct cdata_t *cdata = (struct cdata_t *)arg;

	printk(KERN_ALERT "cdata: wake up current process\n");

	cdata->index = 0;
	wake_up(&cdata->writeable);
}

// Reading: https://www.ibm.com/developerworks/library/l-kernel-memory-access/
static ssize_t cdata_write(struct file *filp, const char __user *ptr, size_t size, loff_t *offset)
{
	DECLARE_WAITQUEUE(wait, current);
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	u8 *buf = cdata->buf;
	unsigned int index = cdata->index;
	struct timer_list *timer = &cdata->timer;
	int i;

//         if (mutex_lock_interruptible(&cdata->write_lock))
//                 return -EINTR;

	for (i = 0; i < size; i++) {
#if 0
		wait_event_interruptible(cdata->writeable, !(index >= 15) );
#else
		if (index >= 15) {
repeat:
			printk(KERN_ALERT "cdata: no space in the buffer\n");

			timer->expires = jiffies + 5 * HZ;
			timer->function = write_data;
			timer->data = (unsigned long)cdata;
			add_timer(timer);

			prepare_to_wait(&cdata->writeable, &wait, TASK_INTERRUPTIBLE);
			
			schedule();

			finish_wait(&cdata->writeable, &wait);

			index = cdata->index;
			printk(KERN_ALERT "cdata: index=%s\n", index);

			if (index > 0)
				goto repeat;
		}
#endif

		copy_from_user(&buf[index], &ptr[i], 1);
		index++;
	}

	cdata->index = index;

	return 0;
}

static struct file_operations cdata_fops = {	
	owner:		THIS_MODULE,
	open:		cdata_open,
	release:	cdata_release,
	compat_ioctl:	cdata_ioctl,
	write:		cdata_write,
};

int cdata_init_module(void)
{
	if (register_chrdev(CDATA_MAJOR, "cdata", &cdata_fops)) {
	    printk(KERN_ALERT "cdata module: can't registered.\n");
	    return -1;
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

MODULE_LICENSE("GPL");
