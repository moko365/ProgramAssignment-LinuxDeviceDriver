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
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#ifdef CONFIG_SMP
#define __SMP__
#endif

#define	BUF_SIZE	64

struct cdata_t {
	unsigned int	count;
	unsigned char 	buf[BUF_SIZE];

	wait_queue_head_t	wq;
	struct timer_list	timer;
};

static DEFINE_MUTEX(mutex);

static int cdata_open(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata;

	cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);

	cdata->count = 0;
	init_waitqueue_head(&cdata->wq);
	init_timer(&cdata->timer);

	filp->private_data = (void *)cdata;

	return 0;
}

static int cdata_ioctl(struct inode *inode, struct file *filp, 
			unsigned int cmd, unsigned long arg)
{
	return 0;
}

static ssize_t cdata_read(struct file *filp, char *buf, 
				size_t size, loff_t *off)
{
	return 0;
}

void flush_lcd(unsigned long priv)
{
	struct cdata_t *cdata = (struct cdata_t *)priv;
	unsigned int count = cdata->count;
	unsigned char *buf = &cdata->buf;

	cdata->count = 0;

	wake_up(&cdata->wq);
}

static ssize_t cdata_write(struct file *filp, const char *user, 
				size_t size, loff_t *off)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	unsigned int count; 
	unsigned char *buf;
	struct timer_list *timer;
	DECLARE_WAITQUEUE(wait, current);

	int i;

	mutex_lock(&mutex);

	count = cdata->count;
	buf = &cdata->buf;
	timer = &cdata->timer

	for (i = 0; i < size; i++) {
		copy_from_user(&buf[count], &user[i], 1);

		count++;

		if (count >= BUF_SIZE) {
			printk(KERN_ALERT "cdata: buffer is full\n");

			timer->expires = jiffies + 1;
			timer->function = flush_lcd;
			timer->data = (unsigned long)cdata;

			add_timer(timer);

			add_wait_queue(&cdata->wq, &wait);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();

			count = cdata->count;
		}
	}

	cdata->count = count;

	mutex_unlock(&mutex);

	return 0;
}

static int cdata_release(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	unsigned int count = cdata->count;
	unsigned char *buf = &cdata->buf;

	buf[count] = '\0';

	printk(KERN_ALERT "cdata: buf = %s\n", buf);

	return 0;
}

static int cdata_mmap(struct file *filp, 
			struct vm_area_struct *vma) 
{
	unsigned long from;
	unsigned long to;
	unsigned long size;

	from = vma->vm_start;
	to = 0x33f00000;
	size = vma->vm_end-vma->vm_start;

	while (size) {
	    remap_page_range(from, to, PAGE_SIZE, PAGE_SHARED);

	    from += PAGE_SIZE;
	    to += PAGE_SIZE;
	    size -= PAGE_SIZE;
	}

	printk(KERN_ALERT "vma start: %p\n", vma->vm_start);
	printk(KERN_ALERT "vma end: %p\n", vma->vm_end);

	return 0;
}

struct file_operations cdata_fops = {	
	owner:		THIS_MODULE,
	open:		cdata_open,
	release:	cdata_release,
	ioctl:		cdata_ioctl,
	read:		cdata_read,
	mmap:		cdata_mmap,
	write:		cdata_write,
};

static struct miscdevice cdata_misc = {
	minor:	20,
	name:	"cdata-fb",
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
