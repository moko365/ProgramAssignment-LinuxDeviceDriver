#ifndef _CDATA_IOCTL_H_
#define	_CDATA_IOCTL_H_

#include <linux/ioctl.h>

#define	CDATA_SYNC	_IOW(0xCE, 1, int)
#define	CDATA_EMPTY	_IO(0xCE, 2)
#define	CDATA_WRITE	_IOW(0xCE, 2, char)

#endif
