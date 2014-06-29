/*
 * omap34xx_sht7x.c - Linux kernel module for SHT7x hardware monitoring
 *
 * Copyright (C) 2011 Moko365 Inc.
 *
 * Written by Jollen Chen <jollen@moko365.com>
 *
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>

struct sht7x_data {
	struct device 		*hwmon_dev;	// Linux hardware minotoring
	struct mutex 		lock;		// Semphoare variable
	const char 		*name;
	char 			valid;
	unsigned long 		last_updated;
	u32 			temperature;	// Temperature
	u32 			humidity;	// Humidity
	u16			valueT;
	u16			valueH;
};

static struct platform_device omap34xx_sht7x_device = {
	.name 	= "omap34xx_sht7x",
	.id	= -1,
};

/*********************** internal ******************************/

static  void set_gpio_184_output()
{
}

static  void set_gpio_185_output()
{
}

static  void set_gpio_184_input()
{
}

static  void set_gpio_185_input()
{
}

/* GPIO High/Low */
static  void gpio_184_high()
{
}

static  void gpio_185_high()
{
}

static  void gpio_184_low()
{
}

static  void gpio_185_low()
{
}

/* GPIO 185 Data In */
static  u32 gpio_185_read()
{
}


//#define	SHT_SCL	(1<<17)
//#define	SHT_SDA (1<<16)

#define SHT_PORT_IN	  gpio_185_read()

#define SHT_SCL_HIGH()    do{gpio_184_high();} while(0)
#define SHT_SCL_LOW()     do{gpio_184_low();} while(0)
#define SHT_SDA_HIGH()    do{gpio_185_high();} while(0)
#define SHT_SDA_LOW()     do{gpio_185_low();} while(0)


/*Delay Function*/
/*   Delay time = (13 + 4*value) * 1000us -> 3.82us*value    */
#define delay_sht()	  udelay(7)

static inline void pulse_sht(void)
{
   SHT_SCL_HIGH();    //CLOCK pulse -> approx 1.67us;
   delay_sht();
   SHT_SCL_LOW();
   delay_sht();
}

typedef enum {
	SHT_MEASURE_TEMP	= 0x03,
	SHT_MEASURE_HUMI	= 0x05,
	SHT_SOFT_RESET		= 0x1e,
} SHTXX_CMD_e;

static void shtxx_Initial(void)
{
   u8 i;

   // Connection reset sequence
   SHT_SDA_HIGH();
   delay_sht();
   SHT_SCL_LOW();
   delay_sht();

   for(i = 0; i < 9; i++){
       SHT_SCL_HIGH();
       delay_sht();
       SHT_SCL_LOW();
       delay_sht();
       //delay 1ms
       udelay(1000);
   }
}

static void shtxx_start_transmission(void)
{
       delay_sht();
       SHT_SDA_HIGH();
       delay_sht();
       SHT_SCL_LOW();           //Initial state
       delay_sht();
       SHT_SCL_HIGH();
       delay_sht();
       SHT_SDA_LOW();
       delay_sht();
       SHT_SCL_LOW();
       delay_sht();
       SHT_SCL_HIGH();
       delay_sht();
       SHT_SDA_HIGH();
       delay_sht();
       SHT_SCL_LOW();
       delay_sht();
       //delay 1ms
#if 0
    // use HR timer
#else
    udelay(1000); 
#endif
}

void sht_sync()
{
    //spin_lock();
    //omap34xx_sht7x->sync = 1;
    //spin_unlock();
    //wake_up();
}

u8 shtxx_write_byte(unsigned char output)
{
   u32 sWait;
   u8 i;

   for(i = 0; i < 8; i++)
   {
       if(output & 0x80)
           SHT_SDA_HIGH();
       else
           SHT_SDA_LOW();
       delay_sht();
       output <<= 1;
       pulse_sht();
       delay_sht();
   }
   SHT_SDA_HIGH();                     //release DATA-line
   delay_sht();
   pulse_sht();
   delay_sht();
   SHT_SDA_HIGH();
   delay_sht();

   sWait = 4000000;                    //Avoid infinity loop
   while(sWait-- > 0){
       if(!(SHT_PORT_IN)){     //!!! Wait for sensing finish
           return true;
       }
   }

   return false;
}

u8 shtxx_write_byte_no_wait(unsigned char output)
{
   u8 i;
   for(i = 0; i < 8; i++)
   {
       if(output & 0x80)
           SHT_SDA_HIGH();
       else
           SHT_SDA_LOW();
       delay_sht();
       output <<= 1;
       pulse_sht();
       delay_sht();
   }
   SHT_SDA_HIGH();                //release DATA-line
   delay_sht();
   pulse_sht();
   delay_sht();
   SHT_SDA_HIGH();
   delay_sht();
   return true;
}

int shtxx_read_word(void)
{
   unsigned int input = 0;
   u8 i;

   SHT_SDA_HIGH();
   for(i = 0; i < 16; i++)
   {
       delay_sht();
       if (SHT_PORT_IN)
           input |= 0x0001;
       if (i != 15)
           input <<= 1;
       if (i == 7)
       {
           pulse_sht();
           delay_sht();
           SHT_SDA_LOW();
           delay_sht();
           pulse_sht();
           delay_sht();
           SHT_SDA_HIGH();     //release the dataline
           delay_sht();
       }
       else
       {
           pulse_sht();
           delay_sht();
       }
   }
   SHT_SDA_HIGH();             //ACK ...........no CRC
   delay_sht();
   pulse_sht();
   delay_sht();

   return input;
}

static void shtxx_read_TH(struct sht7x_data *pResTH)
{
   u8 resT, resH;

   /* 1. Measure Temperature */
   shtxx_start_transmission();                     //Start Transmission

repeat:
   //spin_unlock();
   //prepare_to_wait();
   //sleep_interruptible();
   //spin_lock();

   //if (!pResTh->sync)
   //    goto repeat;

   //spin_unlock();

   resT = shtxx_write_byte(SHT_MEASURE_TEMP);      //Write commmand
   if(resT == true){
       pResTH->valueT = shtxx_read_word();         //Read Temperature
   }

   /* 2. Measure Humidity */
   shtxx_start_transmission();                     //Start transmission with sensor
   resH = shtxx_write_byte(SHT_MEASURE_HUMI);      //Write commmand
   if(resH == true){
       pResTH->valueH = shtxx_read_word();         //Read Humidity 
   }
   /* 3. Convert result */
   //¨ìap¼h¦b°µ¹Bºâ¡A¦]¬°Driver¤£¤ä´©¯BÂI¼Æ
   //shtxx_convert_TH(resT, resH, valueT, valueH, pResTH);

   if(resT && resH){
       pResTH->temperature = 0;
       pResTH->humidity    = 0;
   }
   else{
       pResTH->valueT      = 0;
       pResTH->valueH      = 0;
       pResTH->temperature = 0xFFFF;
       pResTH->humidity    = 0xFFFF;
   }
}


/***************************************************************/

static void sht7x_update(struct sht7x_data *sht7x)
{
	u32 temp_sensor_reg;

	mutex_lock(&sht7x->lock);

	if (time_after(jiffies, sht7x->last_updated + HZ / 2) || !sht7x->valid) {
		shtxx_read_TH(sht7x);

		sht7x->last_updated = jiffies;
		sht7x->valid = 1;
	}

	mutex_unlock(&sht7x->lock);
}

/**
 * show_name - 
 * @dev: 
 * @devattr: 
 * @buf: 
 *
 * Returns 0 on success, else negative errno.
 */
static ssize_t show_name(struct device *dev,
			struct device_attribute *devattr, char *buf)
{
	struct sht7x_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", data->name);
}

/**
 * show_temp - 
 * @dev: 
 * @devattr: 
 * @buf: 
 *
 * Returns 0 on success, else negative errno.
 */
static ssize_t show_temp(struct device *dev,
			 struct device_attribute *devattr, char *buf)
{
	struct sht7x_data *data = dev_get_drvdata(dev);

	sht7x_update(data);

	return sprintf(buf, "%d\n", data->temperature);
}

/**
 * show_humidity - 
 * @dev: 
 * @devattr: 
 * @buf: 
 *
 * Returns 0 on success, else negative errno.
 */
static ssize_t show_humidity(struct device *dev,
			 struct device_attribute *devattr, char *buf)
{
	struct sht7x_data *data = dev_get_drvdata(dev);

	sht7x_update(data);

	return sprintf(buf, "%d\n", data->humidity);
}

static SENSOR_DEVICE_ATTR_2(temp1_input, S_IRUGO, show_temp, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(humidity1_input, S_IRUGO, show_humidity, NULL, 0, 0);
static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);

static int __devinit omap34xx_sht7x_probe(struct platform_device *pdev)
{
	int err;
	struct sht7x_data *data;

	data = kzalloc(sizeof(struct sht7x_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit_platform;
	}

	/* first initialized */
	data->last_updated = jiffies;
	shtxx_Initial();

	dev_set_drvdata(&omap34xx_sht7x_device.dev, data);
	mutex_init(&data->lock);
	data->name = "omap34xx_sht7x";

	err = device_create_file(&omap34xx_sht7x_device.dev,
				 &sensor_dev_attr_temp1_input.dev_attr);
	if (err)
		goto exit_free;

	err = device_create_file(&omap34xx_sht7x_device.dev,
				 &sensor_dev_attr_humidity1_input.dev_attr);
	if (err)
		goto exit_remove;

	err = device_create_file(&omap34xx_sht7x_device.dev, &dev_attr_name);
	if (err)
		goto exit_remove_humidity;

	data->hwmon_dev = hwmon_device_register(&omap34xx_sht7x_device.dev);

	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_all;
	}

        printk(KERN_INFO "omap34xx_sht7x: initialized.\n");

	return 0;

exit_remove_all:
	device_remove_file(&omap34xx_sht7x_device.dev,
			   &dev_attr_name);
exit_remove_humidity:
	device_remove_file(&omap34xx_sht7x_device.dev,
			   &sensor_dev_attr_humidity1_input.dev_attr);
exit_remove:
	device_remove_file(&omap34xx_sht7x_device.dev,
			   &sensor_dev_attr_temp1_input.dev_attr);
exit_free:
	kfree(data);
exit:
exit_platform:
	return err;
}

static int __devexit omap34xx_sht7x_remove(struct platform_device *pdev)
{
	struct sht7x_data *data;
	
	data = dev_get_drvdata(&omap34xx_sht7x_device.dev);

	hwmon_device_unregister(data->hwmon_dev);
	device_remove_file(&omap34xx_sht7x_device.dev, &dev_attr_name);
	device_remove_file(&omap34xx_sht7x_device.dev,
			   &sensor_dev_attr_humidity1_input.dev_attr);
	device_remove_file(&omap34xx_sht7x_device.dev,
			   &sensor_dev_attr_temp1_input.dev_attr);
	kfree(data);
}

static struct platform_driver omap34xx_sht7x_driver = {
	.probe		= omap34xx_sht7x_probe,
	.remove		= omap34xx_sht7x_remove, //  __devexit_p()
	.driver		= {
		.name	= "omap34xx_sht7x",
		.owner	= THIS_MODULE,
	},
};

static int __init omap34xx_sht7x_init(void)
{
	int err;

	err = platform_device_register(&omap34xx_sht7x_device);
	if (err) {
		printk(KERN_ERR
			"Unable to register omap34xx SHT7x humidity and temperature sensor\n");
		goto exit_platform;
	}

	return platform_driver_register(&omap34xx_sht7x_driver);

exit_platform:
	return err;
}

static void __exit omap34xx_sht7x_exit(void)
{
	platform_driver_unregister(&omap34xx_sht7x_driver);
	platform_device_unregister(&omap34xx_sht7x_device);
}

MODULE_AUTHOR("Jollen Chen");
MODULE_DESCRIPTION("SHT7x humidity and temperature sensor for OMAP34xx");
MODULE_LICENSE("GPL");

module_init(omap34xx_sht7x_init)
module_exit(omap34xx_sht7x_exit)
