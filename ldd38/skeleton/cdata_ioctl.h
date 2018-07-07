#ifndef _CDATA_IOCTL_H_
#define	_CDATA_IOCTL_H_

#include <linux/ioctl.h>

#define	CDATA_EMPTY	_IO(0xCE, 1)
#define	CDATA_SYNC	_IO(0xCE, 2)
#define	CDATA_NAME	_IOW(0xCE, 3, char)

#endif
