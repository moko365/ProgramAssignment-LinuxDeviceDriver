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
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

struct pci_device_id vga_pci_tbl[] = {
	{0x80ee, 0xbeef, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};

MODULE_DEVICE_TABLE(pci, vga_pci_tbl);

unsigned int video_base;
unsigned char *video_vbase;

int vga_probe(struct pci_dev *dev, const struct pci_device_id *id) 
{
	unsigned int len;
	int i;
	u16 vendorID;

 	if (pci_enable_device(dev))
	    return -EIO;

	pci_read_config_word(dev, 0x00, &vendorID);

	video_base = pci_resource_start(dev, 0);
	
	len = pci_resource_len(dev, 0);

	printk(KERN_ALERT "probe_pci: vga found. fb = %08x, size = %d\n",
				video_base, len);

	video_vbase = ioremap(video_base, len);

	/* TESTING */
	for (i = 0; i < 100000; i++) {
	    writeb(0x33, video_vbase+i);
	}

	return 0;
}

void vga_remove(struct pci_dev *dev) {
}

static struct pci_driver vga_fb = {
	name:		"vga-fb",
	id_table:	vga_pci_tbl,
	probe:		vga_probe,
	remove:		vga_remove,
};

int probe_pci_init_module(void)
{
	pci_register_driver(&vga_fb);

	return 0;
}

void probe_pci_cleanup_module(void)
{
	pci_unregister_driver(&vga_fb);
}

module_init(probe_pci_init_module);
module_exit(probe_pci_cleanup_module);

MODULE_LICENSE("GPL");

