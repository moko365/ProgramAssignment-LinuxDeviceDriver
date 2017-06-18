#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "cdata_ioctl.h"

int main(void)
{
    int fd;
    pid_t child;
    int i;

    //child = fork();

    fd = open("/dev/cdata", O_RDWR);
    ioctl(fd, IOCTL_EMPTY, 0);
    close(fd);
}
