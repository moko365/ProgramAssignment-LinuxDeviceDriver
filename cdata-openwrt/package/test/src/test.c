#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "cdata_ioctl.h"

int main(void)
{
    int fd;
    int i;
    pid_t child;

    fd = open("/dev/cdata-misc", O_RDWR);
    child = fork();

    if (child != 0) {
	while (1) {
           write(fd, "h", 1);
           write(fd, "e", 1);
           write(fd, "l", 1);
           write(fd, "l", 1);
           write(fd, "o", 1);
	}
    } else {
	while (1) {
        write(fd, "12345", 5);
        write(fd, "12345", 5);
        write(fd, "12345", 5);
        write(fd, "12345", 5);
        write(fd, "12345", 5);
	}
    }

    close(fd);
}
