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
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>
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

#define FIFO_SIZE   BUFSIZE

struct cdata_t {
    char        *iomem;
    unsigned int offset;
    struct work_struct    work;
    wait_queue_head_t   wait;
    struct kfifo *in_fifo;
    spinlock_t fifo_lock;
};

static DEFINE_MUTEX(mutex);

// Supporting APIs
void flush_lcd(struct work_struct *work)
{
    struct cdata_t *cdata = container_of(work, struct cdata_t, work);
    char *fb = cdata->iomem;
    struct kfifo *in_fifo = cdata->in_fifo;
    unsigned char buf[FIFO_SIZE];
    unsigned int offset = cdata->offset;
    unsigned int len;
    int i;

    len =  kfifo_len(in_fifo);
    kfifo_get(in_fifo, buf, len);

    for (i = 0; i < len; i++) {
        writeb(buf[i], fb + offset);
        offset++;

        if (offset >= LCD_SIZE)
            offset = 0;
    }

    cdata->offset = offset;

    // Wake up process
    current->state = TASK_RUNNING;
    wake_up(&cdata->wait);
}

static int cdata_open(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata;

    cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);

    init_waitqueue_head(&cdata->wait);
    cdata->iomem = ioremap(0xe0000000, LCD_SIZE);
    cdata->in_fifo = kfifo_alloc(FIFO_SIZE, GFP_KERNEL, &cdata->fifo_lock);
    INIT_WORK(&cdata->work, flush_lcd);
    mutex_init(&mutex);

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
    struct kfifo *in_fifo = cdata->in_fifo;
    DECLARE_WAITQUEUE(wait, current);
    unsigned char in_buf[4];
    int i;

    for (i = 0; i < count; i++) {
        if (kfifo_len(in_fifo) >= FIFO_SIZE) {
            add_wait_queue(&cdata->wait, &wait);
            current->state = TASK_INTERRUPTIBLE;

            schedule_work(&cdata->work);

            schedule();

            current->state = TASK_RUNNING;
            remove_wait_queue(&cdata->wait, &wait);
        }

        if (copy_from_user(&in_buf[0], &buf[i], 1))
            return -EFAULT;
	kfifo_put(in_fifo, in_buf, 1);
    }

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
    
    return 0;
}

struct file_operations cdata_fops = {	
    open:	cdata_open,
    release:	cdata_release,
    ioctl:	cdata_ioctl,
    read:	cdata_read,
    write:	cdata_write,
    mmap:       cdata_mmap,
};

int my_init_module(void)
{
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
