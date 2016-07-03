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


#define wait_down_int()	{ ADCTSC = DOWN_INT | XP_PULL_UP_EN | \
				XP_AIN | XM_HIZ | YP_AIN | YM_GND | \
				XP_PST(WAIT_INT_MODE); }
#define wait_up_int()	{ ADCTSC = UP_INT | XP_PULL_UP_EN | XP_AIN | XM_HIZ | \
				YP_AIN | YM_GND | XP_PST(WAIT_INT_MODE); }
#define mode_x_axis()	{ ADCTSC = XP_EXTVLT | XM_GND | YP_AIN | YM_HIZ | \
				XP_PULL_UP_DIS | XP_PST(X_AXIS_MODE); }
#define mode_x_axis_n()	{ ADCTSC = XP_EXTVLT | XM_GND | YP_AIN | YM_HIZ | \
				XP_PULL_UP_DIS | XP_PST(NOP_MODE); }
#define mode_y_axis()	{ ADCTSC = XP_AIN | XM_HIZ | YP_EXTVLT | YM_GND | \
				XP_PULL_UP_DIS | XP_PST(Y_AXIS_MODE); }
#define start_adc_x()	{ ADCCON = PRESCALE_EN | PRSCVL(49) | \
				ADC_INPUT(ADC_IN5) | ADC_START_BY_RD_EN | \
				ADC_NORMAL_MODE; \
			  ADCDAT0; }
#define start_adc_y()	{ ADCCON = PRESCALE_EN | PRSCVL(49) | \
				ADC_INPUT(ADC_IN7) | ADC_START_BY_RD_EN | \
				ADC_NORMAL_MODE; \
			  ADCDAT1; }
#define disable_ts_adc()	{ ADCCON &= ~(ADCCON_READ_START); }

static int adc_state = 0;
#define	PEN_UP 0
#define PEN_DOWN 1;
static int pen_state = PEN_UP;

struct cdata_ts {
	struct input_dev ts_input;
	int x;
	int y;
	spinlock_t lock;
};

void cdata_ts_handler(int irq, void *priv, struct pt_regs *reg);
void cdata_bh(unsigned long);
DECLARE_TASKLET(my_tasklet, cdata_bh, NULL);

static void s3c2410_get_XY(void *priv)
{
	struct cdata_ts *cdata = (struct cdata_ts *)priv;

	if (adc_state == 0) { 
		adc_state = 1;
		disable_ts_adc();
		spin_lock_irq(&cdata->lock);
		cdata->y = (ADCDAT0 & 0x3ff); 
		spin_unlock_irq(&cdata->lock);
		mode_y_axis();
		start_adc_y();
	} else if (adc_state == 1) { 
		adc_state = 0;
		disable_ts_adc();
		spin_lock_irq(&cdata->lock);
		cdata->x = (ADCDAT1 & 0x3ff); 
		spin_unlock_irq(&cdata->lock);
		pen_state = PEN_DOWN;

	        my_tasklet.data = (unsigned long)cdata;
	    	tasklet_schedule(&my_tasklet);

		wait_up_int();
	}
}

static int ts_input_open(struct input_dev *dev)
{
	return 0;	
}

static void ts_input_close(struct input_dev *dev)
{
}

void cdata_ts_handler(int irq, void *priv, struct pt_regs *reg)
{
	/* read (x,y) from ADC */
	if (pen_state == PEN_UP) { /* NOW is down */
	    adc_state = 0;
	    mode_x_axis();
	    start_adc_x();
	} else { /* NOW is up */
	    pen_state = PEN_UP;
	    wait_down_int();
	}
}

void cdata_bh(unsigned long priv)
{
	struct cdata_ts *cdata = (struct cdata_ts *)priv;
	struct input_dev *dev = &cdata->ts_input;
	unsigned long flags;

	spin_lock_irqsave(&cdata->lock, flags);

	input_report_abs(dev, ABS_X, cdata->x);
	input_report_abs(dev, ABS_Y, cdata->y);
	input_report_abs(dev, BTN_TOUCH, 1);

	spin_unlock_irqrestore(&cdata->lock, flags);
}

static void s3c2410_isr_adc(int irq, void *priv, struct pt_regs *reg) 
{
	struct cdata_ts *cdata = (struct cdata_ts *)priv;

	if (pen_state == PEN_UP)
	    s3c2410_get_XY((void *)cdata);
}

static int cdata_ts_open(struct inode *inode, struct file *filp)
{
    struct cdata_ts *cdata;
    int ret;

    printk(KERN_INFO "cdata_ts_open");

    cdata = kmalloc(sizeof(struct cdata_ts), GFP_KERNEL);

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

	/* Enable ADC interrupt */
	ret = request_irq(IRQ_ADC_DONE, s3c2410_isr_adc, SA_INTERRUPT, 
			  "cdata-adc", (void *)cdata);
	if (ret) goto adc_failed;

	/* Request touch panel IRQ */
	ret = request_irq(IRQ_TC, cdata_ts_handler, SA_INTERRUPT, 
		"cdata-ts", (void *)cdata);
	if (ret) goto ts_failed;

	/* Wait for touch screen interrupts */
	wait_down_int();

	return 0;
adc_failed:
	printk(KERN_ALERT "cdata: request ADC irq failed.\n");
	return ret;
ts_failed:
	printk(KERN_ALERT "cdata: request TS irq failed.\n");
	return ret;
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

#define CDATA_TS_MINOR 39

static struct miscdevice cdata_ts_misc = {
	minor:		CDATA_TS_MINOR,
	name:		"cdata-ts",
	fops:		&cdata_ts_fops,
};

int cdata_ts_init_module(void)
{
	set_gpio_ctrl(GPIO_YPON); 
	set_gpio_ctrl(GPIO_YMON);
	set_gpio_ctrl(GPIO_XPON);
	set_gpio_ctrl(GPIO_XMON);

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

