#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#ifdef CONFIG_SMP
#define __SMP__
#endif

#define	CDATA_MAJOR 121 
#define BUFSIZE     1024

#define LCD_WIDTH   (640)
#define LCD_HEIGHT  (480)
#define LCD_BPP     (1)
#define LCD_SIZE    (LCD_WIDTH*LCD_HEIGHT*LCD_BPP)

struct cdata_t {
    char        data[BUFSIZE];
    int         index;
    char        *iomem;
    unsigned int    offset;
#if 0
    struct timer_list   timer;
#endif

    struct work_struct    work;
    wait_queue_head_t   wait;
};

static DECLARE_MUTEX(cdata_sem);

// Supporting APIs
void flush_lcd(struct work_struct *work)
{
    struct cdata_t *cdata = container_of(work, struct cdata_t, work);
    char *fb = cdata->iomem;
    int offset = cdata->offset;
    int index = cdata->index;
    int i;

    for (i = 0; i < index; i++) {
        writeb(cdata->data[i], fb + offset);
        offset++;

        if (offset >= LCD_SIZE)
            offset = 0;
    }

    cdata->index = 0;
    cdata->offset = offset;

    // Wake up process
    //current->state = TASK_RUNNING;
    wake_up(&cdata->wait);
}

static int cdata_open(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata;

    cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
    cdata->index = 0;

    init_waitqueue_head(&cdata->wait);
    cdata->iomem = ioremap(0xe0000000, LCD_SIZE);
#if 0
    init_timer(&cdata->timer);
#endif
    INIT_WORK(&cdata->work, flush_lcd);
    cdata->offset = 0;

    filp->private_data = (void *)cdata;

	return 0;
}

static int cdata_ioctl(struct inode *inode, struct file *filp, 
			unsigned int cmd, unsigned long arg)
{
	printk(KERN_ALERT "cdata: in cdata_ioctl()\n");

    return 0;
}

static ssize_t cdata_read(struct file *filp, char *buf, 
				size_t size, loff_t *off)
{
	printk(KERN_ALERT "cdata: in cdata_read()\n");
    return 0;
}

static ssize_t cdata_write(struct file *filp, const char *buf, 
				size_t count, loff_t *off)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
    DECLARE_WAITQUEUE(wait, current);
    int i;

    down(&cdata_sem);
    for (i = 0; i < count; i++) {
        if (cdata->index >= BUFSIZE) {
            add_wait_queue(&cdata->wait, &wait);
            current->state = TASK_INTERRUPTIBLE;

            /*
             */
#if 0
            cdata->timer.expires = jiffies + 500;
            cdata->timer.function = flush_lcd;
            cdata->timer.data = (unsigned long)cdata;
            add_timer(&cdata->timer);
#endif
            schedule_work_on(1, &cdata->work);

            schedule();

            current->state = TASK_RUNNING;
            remove_wait_queue(&cdata->wait, &wait);
        }

        if (copy_from_user(&cdata->data[cdata->index++], &buf[i], 1))
            return -EFAULT;
    }
    up(&cdata_sem);

	return 0;
}

static int cdata_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "cdata: in cdata_release()\n");
	return 0;
}

static int cdata_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long start = vma->vm_start;
    unsigned long end = vma->vm_end;
    unsigned long size;

    size = end - start;
    //remap_page_range(start, 0x33f00000, size, PAGE_SHARED);
}

struct file_operations cdata_fops = {	
	open:		cdata_open,
	release:	cdata_release,
	ioctl:		cdata_ioctl,
	read:		cdata_read,
	write:		cdata_write,
    mmap:       cdata_mmap,
};

int my_init_module(void)
{
    int i;

	if (register_chrdev(CDATA_MAJOR, "cdata", &cdata_fops)) {
	    printk(KERN_ALERT "cdata module: can't registered.\n");
    }

	return 0;
}

void my_cleanup_module(void)
{
	unregister_chrdev(CDATA_MAJOR, "cdata");
	printk(KERN_ALERT "cdata module: unregisterd.\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);

MODULE_LICENSE("GPL");
