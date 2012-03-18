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
//#include <linux/config.h>
#include <linux/tqueue.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>

#include "cdata_ioctl.h"

#define	LCD_WIDTH	(240)
#define	LCD_HEIGHT	(320)
#define	LCD_BPP		(4)
#define	LCD_LENGTH	(LCD_WIDTH*LCD_HEIGHT*LCD_BPP)
#define	LCD_SIZE	(LCD_WIDTH*LCD_HEIGHT)

#define	BUF_LENGTH	(16384)

struct cdata_t {
	unsigned char 	*fb;		/* base address */
	unsigned int 	fb_cur;		/* current pixel */
	unsigned char	*buf;	
	unsigned int	buf_idx;	/* buffer index */	
	
	wait_queue_head_t	wq;

	struct timer_list 	timer;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	struct work_struct	work;	// 2.6
#else
	struct tq_struct	tq;	// 2.4
#endif
	struct semaphore sem_wait;
};

static void lcd_write(void *);

#ifndef	MODULE
static int 	delay;
/**
 * cdata=x,y
 */
int __init cdata_setup(char *str)
{
	int ints[2];

	str = get_options(str, ARRAY_SIZE(ints), ints);

	if (ints[0] == 1)
	    delay = 1;
	return 0;
}

__setup("cdata=", cdata_setup);
#endif

/*** I/O wrapper functions *****/

/**
 * TODO must be reentrant
 */
static int cdata_open(struct inode *inode, struct file *filp)
{
	unsigned int n;
	struct cdata_t *cdata;

#if 0
	/* TODO reentrancy */
 	if (cdata_u_count && 
   	   (cdata_u_owner != current->uid) && /* allow user */
   	   (cdata_u_owner != current->euid)) /* allow su */
     	        return -EBUSY;  
 	}

 	if (cdata_u_count != 0) return -1;

 	if (cdata_u_count == 0)
   	    cdata_u_owner = current->uid;
 	cdata_u_count++;
#endif

	cdata = (struct cdata_t *)
			kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
	cdata->buf = (unsigned char *)
			kmalloc(BUF_LENGTH, GFP_KERNEL);

	n = MINOR(inode->i_rdev);

	printk(KERN_ALERT "cdata: cdata_open\n");
	printk(KERN_ALERT "cdata: minor = %d\n", n);

        /* draw LCD panel at 0x33F00000 */
	cdata->fb = ioremap(0x33f00000, LCD_LENGTH);
	cdata->fb_cur = 0;
	cdata->buf_idx = 0;	// empty buffer

	init_waitqueue_head(&cdata->wq);

	init_timer(&cdata->timer);

	//INIT_TQUEUE(&cdata->tq, lcd_write, (void *)cdata);
	INIT_WORK(&cdata->work, lcd_write);

	filp->private_data = (void *)cdata;

	return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
	struct	cdata_t	*cdata = (struct cdata_t *)filp->private_data;

	kfree(cdata->buf);
	kfree(cdata);

	del_timer(&cdata->timer);

	return 0;
}

/* is it reentrant code ? */
static void lcd_write(void * priv)
{
	struct	cdata_t	*cdata = (struct cdata_t *)priv;
	int i, n;
	unsigned char *buf;
	unsigned char *fb;
	unsigned int fb_cur;
	int j;

	//n = cdata->buf_idx;
	buf = cdata->buf;
	fb = cdata->fb;
	fb_cur = cdata->fb_cur;

	for (i = 0; i < BUF_LENGTH; i++) {
	    writeb(buf[i], fb+fb_cur);
	    fb_cur++;

	    if (fb_cur >= LCD_LENGTH)
		fb_cur = 0;

	    /* for debug */
	    if (delay == 1) {
	        schedule();
	        for (j = 0; j < 10000; j++)
		    ;
	    }
	}

	cdata->buf_idx = 0;
	cdata->fb_cur = fb_cur;

	wake_up(&cdata->wq);
}

static ssize_t cdata_write(struct file *filp, const char *buf, 
			size_t size, loff_t *off)
{
	struct	cdata_t	*cdata = (struct cdata_t *)filp->private_data;
	unsigned int idx;
	int i;
	DECLARE_WAITQUEUE(wait, current);

	down(&cdata->sem_wait);
	idx = cdata->buf_idx;
	up(&cdata->sem_wait);

	for (i = 0; i < size; i++) {
	    if (idx >= BUF_LENGTH) {
		//cdata->buf_idx = idx;

		//schedule_task(&cdata->tq); 
		queue_work(&cdata->work);

		/* blocking io */
		spin_lock();
		 add_wait_queue(&cdata->wq, &wait);
		current->state = TASK_UNTERRUPTIBLE;
		spin_unlock();
		schedule();
		//interruptible_sleep_on(&cdata->wq); 

		//idx = cdata->buf_idx;
	    }
	    copy_from_user(&cdata->buf[idx++], &buf[i], 1); // FIXME
	}

	down(&cdata->sem_wait);
	cdata->buf_idx = idx;
	up(&cdata->sem_wait);

	return 0;
}

static int cdata_ioctl(struct inode *inode, struct file *filp, 
				unsigned int cmd, unsigned long arg)
{
	struct	cdata_t	*cdata = (struct cdata_t *)filp->private_data;
	unsigned int	*fb = cdata->fb;
	int 		n;
	unsigned int 	*num = (unsigned int *)arg;

	switch (cmd) {
	    case CDATA_CLEAR:
			for (n = 0; n < num; n++) {
			    writel(0x00000000, fb++);
			}
			cdata->fb_cur = num;
			break;
	    case CDATA_RED:
			for (n = 0; n < LCD_SIZE; n++) {
			    writel(0x00ff0000, fb++);
			}
			cdata->fb_cur = LCD_SIZE;
			break;
	    case CDATA_GREEN:
			for (n = 0; n < LCD_SIZE; n++) {
			    writel(0x0000ff00, fb++);
			}
			cdata->fb_cur = LCD_SIZE;
			break;
	    case CDATA_BLUE:
			for (n = 0; n < LCD_SIZE; n++) {
			    writel(0x000000ff, fb++);
			}
			cdata->fb_cur = LCD_SIZE;
			break;

	    case CDATA_BLACK:
			for (n = 0; n < LCD_SIZE; n++) {
			    writel(0x00ffffff, fb++);
			}
			cdata->fb_cur = LCD_SIZE;
			break;
	    case CDATA_WHITE:
			for (n = 0; n < LCD_SIZE; n++) {
			    writel(0x00000000, fb++);
			}
			cdata->fb_cur = LCD_SIZE;
			break;
	    default:
			return -ENOTTY;
	}

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

static struct file_operations cdata_fops = {	
	owner:		THIS_MODULE,
	open:		cdata_open,
	write:		cdata_write,
	ioctl:		cdata_ioctl,
	mmap:		cdata_mmap,
	release:	cdata_close,
};

static struct miscdevice cdata_misc = {
	minor:	CDATA_MINOR,
	name:	"cdata",
	fops:	&cdata_fops,
};

static int __init s3c2410fb_probe(struct platform_device *pdev)
{
    if (misc_register(&cdata_misc) < 0) {
	printk(KERN_ALERT "cdata: register failed.\n");
	return -1;
    }
}

static struct platform_driver s3c2410fb_driver = {
	.probe		= s3c2410fb_probe,
	.remove		= s3c2410fb_remove,
	.suspend	= s3c2410fb_suspend,
	.resume		= s3c2410fb_resume,
	.driver		= {
		.name	= "s3c2410-lcd",
		.owner	= THIS_MODULE,
	},
};

int __init cdata_fb_init_module(void)
{
    return platform_driver_register(&s3c2410fb_driver);

    printk(KERN_ALERT "cdata: hello day1\n");
    return 0;
}

void __exit cdata_fb_cleanup_module(void)
{
    misc_deregister(&cdata_misc);

    printk(KERN_ALERT "cdata: bye\n");
}

module_init(cdata_fb_init_module);
module_exit(cdata_fb_cleanup_module);

MODULE_LICENSE("GPL");
