#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "cdata_ioctl.h"

int main(void)
{
    int fd;
    pid_t child;

    child = fork();
    fd = open("/dev/cdata-misc", O_RDWR);

    if (child != 0) {
        write(fd, "h", 1);
        write(fd, "e", 1);
	ioctl(fd, IOCTL_SYNC, 0);
        write(fd, "l", 1);
        write(fd, "l", 1);
        write(fd, "o", 1);
	ioctl(fd, IOCTL_SYNC, 0);
    } else {
        write(fd, "12345", 5);
        write(fd, "12345", 5);
        write(fd, "12345", 5);
        write(fd, "12345", 5);
        write(fd, "12345", 5);
    }

    close(fd);
}
