#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "cdata_ioctl.h"

#ifdef CONFIG_SMP
#define __SMP__
#endif

#define    CDATA_MAJOR 121
#define    BUF_SIZE    32

struct cdata_t {
  unsigned char *buf;
  unsigned int idx;
  wait_queue_head_t wq;
  struct timer_list timer;
};

static int
cdata_open (struct inode *inode, struct file *filp)
{
  struct cdata_t *cdata;

  cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);

  cdata->buf = (unsigned char *)kmalloc(BUF_SIZE, GFP_KERNEL);
  cdata->idx = 0;

  init_waitqueue_head(&cdata->wq);
  init_timer(&cdata->timer);

  filp->private_data = (void *)cdata;

  return 0;
}

static int
cdata_release (struct inode *inode, struct file *filp)
{
  struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
  kfree(cdata->buf);
  kfree(cdata);
  return 0;
}

static int
cdata_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
  int idx;

  idx = cdata->idx;
  
  switch (cmd) {
  case IOCTL_WRITE:
    copy_from_user(&cdata->buf[idx++], (char *)arg, 1);
    cdata->idx = idx;
    break;
  default:
    return -ENOTTY;
  }

  return 0;
}

static ssize_t
cdata_read (struct file *filp, char *buf, size_t size, loff_t * off)
{
  struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
  printk (KERN_ALERT "cdata: in cdata_read()\n");
  return 0;
}

void flush_buffer(void *priv)
{
    struct cdata_t *cdata = (struct cdata_t *)priv;

    cdata->idx = 0;
    wake_up(&cdata->wq);
}

static ssize_t
cdata_write (struct file *filp, const char *user, size_t size, loff_t * off)
{
    struct cdata_t *cdata = (struct cdata_t *)filp->private_data;
    unsigned int idx = cdata->idx;
    unsigned char *buf = cdata->buf;
    struct timer_list *timer = &cdata->timer;
#if 0
    DECLARE_WAITQUEUE(wait, current);
#else
    DEFINE_WAIT(wait);
#endif
    int i;

    for (i = 0; i < size; i++) {
        if (idx >= BUF_SIZE) {
            printk(KERN_ALERT "cdata: buffer is full\n");

	    timer->expires = jiffies + 300;
	    timer->function = flush_buffer;
	    timer->data = (unsigned long)cdata;
    	    add_timer(timer);

#if 0 	    /* not atomic */
	    add_wait_queue(&cdata->wq, &wait);
	    set_current_state(TASK_INTERRUPTIBLE);
#else	    /* atomic */
	    prepare_to_wait(&cdata->wq, &wait, TASK_INTERRUPTIBLE);
#endif
	    schedule();
#if 0	    /* not atomic */
	    set_current_state(TASK_RUNNING);
	    remove_wait_queue(&cdata->wq, &wait);
#else	    /* atomic */
	    finish_wait(&cdata->wq, &wait);
#endif
	    idx = cdata->idx;
        }
        copy_from_user(&buf[idx], &user[i], 1);
        idx++;
    }

    cdata->idx = idx;

    return 0;
}

struct file_operations cdata_fops = {
open:cdata_open,
release:cdata_release,
unlocked_ioctl:cdata_ioctl,
read:cdata_read,
write:cdata_write,
};

static struct miscdevice cdata_misc = {
  minor:20,
  name:"cdata",
  fops:&cdata_fops,
};

int
my_init_module (void)
{
  if (misc_register (&cdata_misc)) {
    printk (KERN_ALERT "cdata: register failed\n");
    return -1;
  }

  printk (KERN_ALERT "cdata module: registered.\n");

  return 0;
}

void
my_cleanup_module (void)
{
  misc_deregister (&cdata_misc);
  printk (KERN_ALERT "cdata module: unregisterd.\n");
}

module_init (my_init_module);
module_exit (my_cleanup_module);

MODULE_LICENSE ("GPL");
