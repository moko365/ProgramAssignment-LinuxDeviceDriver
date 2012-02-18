#ifndef _CDATA_IOCTL_H_
#define	_CDATA_IOCTO_H_

#include <linux/ioctl.h>

#define	CDATA_CLEAR	_IOW(0xCE, 1, int)

#define	CDATA_RED	_IO(0xCE, 2)
#define	CDATA_GREEN	_IO(0xCE, 3)
#define	CDATA_BLUE	_IO(0xCE, 4)

#define	CDATA_BLACK	_IO(0xCE, 5)
#define	CDATA_WHITE	_IO(0xCE, 6)

#endif
