#ifndef _CDATA_IOCTL_H_
#define _CDATA_IOCTO_H_

#include <linux/ioctl.h>

#define IOCTL_EMPTY _IO(0xCE, 0)
#define IOCTL_SYNC  _IO(0xCE, 1)
#define IOCTL_NAME  _IOW(0xCE, 2, char *)

#endif
