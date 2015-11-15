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

void cdata_bh(unsigned long);
DECLARE_TASKLET(my_tasklet, cdata_bh, NULL);

struct cdata_ts {
	struct input_dev ts_input;
	int x;
	int y;

	spinlock_t lock;;
};

void cdata_ts_handler(int irq, void *priv, struct pt_regs *reg);

static int ts_input_open(struct input_dev *dev)
{	
	struct cdata_ts *cdata = (struct cdata_ts *)dev->private;
}

static int ts_input_close(struct input_dev *dev)
{
}

void cdata_ts_handler(int irq, void *priv, struct pt_regs *reg)
{
	struct cdata_ts *cdata = (struct cdata_ts *)priv;

	printk(KERN_INFO "data_ts: TH...\n");

	/* FIXME: read (x,y) from ADC */
	cdata->x = 100;
	cdata->y = 100;

	my_tasklet.data = (unsigned long)cdata;

	tasklet_schedule(&my_tasklet);
}

void cdata_bh(unsigned long priv)
{
	struct cdata_ts *cdata = (struct cdata_ts *)priv;
	struct input_dev *dev = &cdata->ts_input;
	unsigned long flags;
	int x, y;

	printk(KERN_INFO "data_ts: down...\n");

	spin_lock_irqsave(&cdata->lock, flags);
	//spin_lock(&cdata->lock);
	//spin_lock_irq(&cdata->lock);

	x = cdata->x;
	y = cdata->y;

	//spin_unlock_irq(&cdata->lock);
	//spin_unlock(&cdata->lock);
	spin_unlock_irqrestore(&cdata->lock, flags);

	input_report_abs(dev, ABS_X, x);
	input_report_abs(dev, ABS_Y, y);
}

static int cdata_ts_open(struct inode *inode, struct file *filp)
{
	struct cdata_ts *cdata;

	cdata = kmalloc(sizeof(struct cdata_ts), GFP_KERNEL);

	set_gpio_ctrl(GPIO_YPON);
	set_gpio_ctrl(GPIO_YMON);
	set_gpio_ctrl(GPIO_XPON);
	set_gpio_ctrl(GPIO_XMON);

	ADCTSC = DOWN_INT | XP_PULL_UP_EN | \
                 XP_AIN | XM_HIZ | YP_AIN | YM_GND | \
                 XP_PST(WAIT_INT_MODE);

	/** handling input device ***/
	cdata->ts_input.name = "cdata-ts";
	cdata->ts_input.open = ts_input_open;
	cdata->ts_input.close = ts_input_close;
    	cdata->ts_input.private = (void *)cdata;

    	// Set events
    	cdata->ts_input.evbit[0] = BIT(EV_ABS);
    	// Set types
	cdata->ts_input.absbit[0] = BIT(ABS_X) | BIT(ABS_Y);

	input_register_device(&cdata->ts_input);

	cdata->x = 0;
	cdata->y = 0;

	filp->private_data = (void *)cdata;

        /* Request touch panel IRQ */
        if (request_irq(IRQ_TC, cdata_ts_handler, 0,
                "cdata-ts", (void *)cdata)) {
            printk(KERN_ALERT "cdata: request irq failed.\n");
            return -1;
        }

	return 0;
}

static ssize_t cdata_ts_read(struct file *filp, char *buf, size_t size, loff_t *off)
{
	return 0;
}

static ssize_t cdata_ts_write(struct file *filp, const char *buf, size_t size, 
			loff_t *off)
{
	return 0;
}

static int cdata_ts_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static int cdata_ts_ioctl(struct inode *inode, struct file *filp, 
		unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static struct file_operations cdata_ts_fops = {	
	owner:		THIS_MODULE,
	open:		cdata_ts_open,
	release:	cdata_ts_close,
	read:		cdata_ts_read,
	write:		cdata_ts_write,
	ioctl:		cdata_ts_ioctl,
};

static struct miscdevice cdata_ts_misc = {
	minor:		CDATA_TS_MINOR,
	name:		"cdata-ts",
	fops:		&cdata_ts_fops,
};

int cdata_ts_init_module(void)
{
	if (misc_register(&cdata_ts_misc) < 0) {
	    printk(KERN_INFO "CDATA-TS: can't register driver\n");
	    return -1;
	}
	printk(KERN_INFO "CDATA-TS: cdata_ts_init_module\n");
	return 0;
}

void cdata_ts_cleanup_module(void)
{
	misc_register(&cdata_ts_misc);
}

module_init(cdata_ts_init_module);
module_exit(cdata_ts_cleanup_module);

MODULE_LICENSE("GPL");

