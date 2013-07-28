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

#define IO_MEM              0x33f00000
#define VGA_MODE_WIDTH      240
#define VGA_MODE_HEIGHT     320
#define VGA_MODE_BPP        32
#define BUF_SIZE            1024

struct cdata_t {
    char *buf;
    unsigned int        index;
    wait_queue_head_t   wq;
    unsigned char        *fbmem;
    unsigned char        *fbmem_start, *fbmem_end;
    struct timer_list   timer;
};

void flush_buffer(unsigned long priv)
{
    struct cdata_t *cdata = (struct cdata_t *)priv;
    unsigned char *ioaddr;
    int i;

    ioaddr = (unsigned char *)cdata->fbmem;

    for (i = 0; i < BUF_SIZE; i++) {
        if (ioaddr >= cdata->fbmem_end)
            ioaddr = cdata->fbmem_start;

        writeb(cdata->buf[i], ioaddr++); 
    }

    cdata->fbmem = ioaddr;
    cdata->index = 0;

    wake_up(&cdata->wq);
}

static int cdata_open(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata;
    int i;

    cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
    cdata->buf = (char *)kmalloc(BUF_SIZE, GFP_KERNEL);

    cdata->index = 0;
    init_waitqueue_head(&cdata->wq);

    cdata->fbmem_start = (unsigned int *) 
            ioremap(IO_MEM, VGA_MODE_WIDTH
                    * VGA_MODE_HEIGHT
                    * VGA_MODE_BPP
                    / 8);

    cdata->fbmem_end = cdata->fbmem_start + VGA_MODE_WIDTH 
                                      * VGA_MODE_HEIGHT
                                      * VGA_MODE_BPP
                                      / 8;

    cdata->fbmem = cdata->fbmem_start;

    init_timer(&cdata->timer);

    filp->private_data = (void *)cdata;

    printk(KERN_INFO "in cdata_open: filp = %08x\n", filp);
    return 0;
}

static ssize_t cdata_read(struct file *filp, char *buf, size_t size, loff_t *off)
{
    return 0;
}

static ssize_t cdata_write(struct file *filp, const char *buf, size_t size, loff_t *off)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
    struct timer_list *timer;
    unsigned int index = cdata->index;
    int i;

    timer = &cdata->timer;

    for (i = 0; i < size; i++) {
        if (index >= BUF_SIZE) {
            printk(KERN_INFO "cdata: buffer full\n");

            timer->expires =  jiffies + 1;
            timer->function = flush_buffer;
            timer->data = (unsigned long)cdata;

            add_timer(timer);

            interruptible_sleep_on(&cdata->wq); //re-scheduling
        }
        copy_from_user(&cdata->buf[index], &buf[i], 1);
        index++;
    }

    cdata->index = index;

    return 0;
}

static int cdata_close(struct inode *inode, struct file *filp)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
    unsigned int index = cdata->index;

    cdata->buf[index] = '\0';

    printk(KERN_INFO "in cdata_close: %s\n", cdata->buf);

    del_timer(&cdata->timer);

    return 0;
}

int cdata_ioctl(struct inode *inode, struct file *filp, 
                    unsigned int cmd, unsigned long arg)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
    unsigned int index = cdata->index;

    switch (cmd) {
        case CDATA_EMPTY:
            index = 0;
            break;
        case CDATA_SYNC:
            cdata->buf[index] = '\0';
            printk(KERN_INFO "str: %s\n", cdata->buf);
            break;
        case CDATA_WRITE:
            cdata->buf[index++] = *((char *)arg);
            break;
        default:
            return -1;
    }

    cdata->index = index;

    return 0;
}

static struct file_operations __cdata_fops = {    
        owner:      THIS_MODULE,
        open:       cdata_open,
        release:    cdata_close,
        read:       cdata_read,
        write:      cdata_write,
        ioctl:      cdata_ioctl,
};

static struct miscdevice cdata_fops = {
        minor:      12,
        name:       "cdata",
        fops:       &__cdata_fops,
};

int cdata_init_module(void)
{
    if (misc_register(&cdata_fops)) {
        printk(KERN_ALERT "cdata: register failed.\n");
        return -1;
    }
    printk(KERN_ALERT "cdata-fb: registered .\n");
}

void cdata_cleanup_module(void)
{
    misc_deregister(&cdata_fops);
}

module_init(cdata_init_module);
module_exit(cdata_cleanup_module);

MODULE_LICENSE("GPL");
