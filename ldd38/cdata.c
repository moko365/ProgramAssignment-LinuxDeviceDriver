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
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "cdata_ioctl.h"

//#undef	__ENABLE_REENTRANT__ 
#define	__ENABLE_REENTRANT__  1

#define CDATA_MAJOR 121
#define	BUF_SIZE 16

static DEFINE_MUTEX(ioctl_lock);
static struct dentry *debugfs;

void *write_framebuffer_with_timer(unsigned long);
void write_framebuffer_with_work(struct work_struct *);

struct cdata_t {
	unsigned char buf[BUF_SIZE];
	int idx;
	wait_queue_head_t writeable;
	struct timer_list timer;
	struct work_struct work;
	struct mutex write_lock;
	spinlock_t lock;
};

static int cdata_open(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata;

	printk(KERN_ALERT "cdata in open: filp = %p\n", filp);

	cdata = kzalloc(sizeof(*cdata), GFP_KERNEL);
	cdata->idx = 0;

	init_waitqueue_head(&cdata->writeable);
	init_timer(&cdata->timer);
	INIT_WORK(&cdata->work, write_framebuffer_with_work);
	mutex_init(&cdata->write_lock);
	spin_lock_init(&cdata->lock);

	filp->private_data = (void *)cdata;

	return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	int idx;
	int i;

	idx = cdata->idx;

	for (i = 0; i < idx; i++) {
		//printk(KERN_ALERT "buf[%d]: %c\n", i, cdata->buf[i]);
	}

	del_timer(&cdata->timer);
	kfree(cdata);
	
	return 0;
}

static ssize_t cdata_read(struct file *filp, const char __user *user, 
	size_t size, loff_t *off)
{
	printk(KERN_ALERT "cdata in read\n");
	return 0;
}

void write_framebuffer_with_work(struct work_struct *work)
{
	struct cdata_t *cdata = container_of(work, struct cdata_t, work);
	cdata->idx = 0;

	printk(KERN_ALERT "cdata: [work queue] wake up current process\n");
	wake_up_interruptible(&cdata->writeable);
}

void *write_framebuffer_with_timer(unsigned long arg)
{
	struct cdata_t *cdata = (struct cdata_t *)arg;
	cdata->idx = 0;

	printk(KERN_ALERT "cdata: wake up current process\n");
	wake_up_interruptible(&cdata->writeable);
}

static ssize_t cdata_write(struct file *filp, const char __user *user, 
	size_t size, loff_t *off)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	DECLARE_WAITQUEUE(wait, current);
	struct timer_list *timer;
	int i;
	int idx;

#ifdef __ENABLE_REENTRANT__
	if (mutex_lock_interruptible(&cdata->write_lock))
		return -EINTR;
#endif
	idx = cdata->idx;
	timer = &cdata->timer;

	for (i = 0; i < size; i++) {
		while (idx > (BUF_SIZE - 1)) {
			printk(KERN_ALERT "cdata: no space in the buffer\n");
			//DEFINE_WAIT(wait);

			//prepare_to_wait(&cdata->writeable, &wait, TASK_INTERRUPTIBLE);
			add_wait_queue(&cdata->writeable, &wait);
			set_current_state(TASK_INTERRUPTIBLE);

#if 0
			timer->expires = jiffies + 1*HZ;
			timer->data = (unsigned long)cdata;
			timer->function = write_framebuffer_with_timer;
			add_timer(timer);
#else
			schedule_work(&cdata->work);
#endif
			if (signal_pending(current)) {
#ifdef __ENABLE_REENTRANT__
				mutex_unlock(&cdata->write_lock);
#endif
				return -ERESTARTSYS;
			}

			schedule();

			finish_wait(&cdata->writeable, &wait);

			idx = cdata->idx;
			printk(KERN_ALERT "[debug] idx = %d\n", idx);
		}
		copy_from_user(&cdata->buf[idx], &user[i], 1);
		idx++;
	}

	cdata->idx = idx;

#ifdef __ENABLE_REENTRANT__
	mutex_unlock(&cdata->write_lock);
#endif
	return 0;
}

static long cdata_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
	char *buf;
	int idx;
	int i;
	int ret = 0;
	char *user;
	int size;

#ifdef __ENABLE_REENTRANT__
	if (mutex_lock_interruptible(&ioctl_lock))
		return -EINTR;
#endif

	user = (char *)arg;
	size = sizeof(*user);

	idx = cdata->idx;
	buf = cdata->buf;

	switch (cmd) {
	case IOCTL_EMPTY:
		idx = 0;
		break;
	case IOCTL_SYNC:
		printk(KERN_ALERT "in ioctl: %s\n", buf);
		break;
	case IOCTL_NAME:
		for (i = 0; i < size; i++) {
			if (idx > (BUF_SIZE - 1)) {
				ret = -EFAULT;
				goto exit;
			}
			copy_from_user(&buf[idx], &user[i], 1);
			idx++;
		}
		break;
	default:
		goto exit;
	}

exit:
	cdata->idx = idx;
#ifdef __ENABLE_REENTRANT__
	mutex_unlock(&ioctl_lock);
#endif
	return ret;
}

static int cdata_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long start = vma->vm_start;
    unsigned long end = vma->vm_end;
    unsigned long size = end - start;

    printk(KERN_ALERT "remap %p to 0xe0000000\n", start);
    remap_pfn_range(vma, start, 0xe0000000, size, PAGE_SHARED);

    return 0;
}

static struct file_operations cdata_fops = {
    owner:      	THIS_MODULE,
    open:		cdata_open,
    read:		cdata_read,
    write:		cdata_write,
    mmap:		cdata_mmap,
    unlocked_ioctl:	cdata_ioctl,
    release:    	cdata_close
};

static struct miscdevice cdata_miscdev = {
	.minor	= 77,
	.name	= "cdata-misc",
	.fops	= &cdata_fops,
};

static int cdata_plat_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = misc_register(&cdata_miscdev);
	if (ret < 0) {
		printk(KERN_ALERT "misc_register failed\n");
		goto exit;
	}

	printk(KERN_ALERT "cdata module: registered!\n");

exit:
	return ret;
}

static int cdata_plat_remove(struct platform_device *pdev)
{
	misc_deregister(&cdata_miscdev);
	printk(KERN_ALERT "cdata module: unregisterd.\n");
}

static struct platform_driver cdata_plat_driver = {
	.probe 			= cdata_plat_probe,
	.remove 		= cdata_plat_remove,
	.driver 		= {
		   .name	= "cdata",
		   .owner	= THIS_MODULE,
	},
};

int cdata_init_module(void)
{
	int ret = 0;

	debugfs = debugfs_create_file("cdata", S_IRUGO, NULL, NULL, &cdata_fops);

	if (IS_ERR(debugfs)) {
		ret = PTR_ERR(debugfs);
		printk(KERN_ALERT "debugfs_create_file failed\n");
		goto exit;
	}

	printk(KERN_ALERT "cdata: debugfs created\n");

	mutex_init(&ioctl_lock);

	ret = platform_driver_register(&cdata_plat_driver);
exit:
	return ret;
}

void cdata_cleanup_module(void)
{
	platform_driver_unregister(&cdata_plat_driver);
	debugfs_remove(debugfs);
}

module_init(cdata_init_module);
module_exit(cdata_cleanup_module);

MODULE_LICENSE("GPL");
