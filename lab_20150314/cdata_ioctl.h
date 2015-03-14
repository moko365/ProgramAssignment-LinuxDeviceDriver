#ifndef _CDATA_IOCTL_H_
#define _CDATA_IOCTL_H_

#include <linux/ioctl.h>

#define IOCTL_NAME  _IOW(0xD0, 1, char *)
#define IOCTL_WRITE  _IOW(0xD0, 1, char *)

#endif
