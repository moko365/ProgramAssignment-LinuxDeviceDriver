/*
 * Filename: test.c
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "cdata_ioctl.h"

int main(void)
{
    int fd;
    int i;

    i = 10000;
    fd = open("/dev/cdata", O_RDWR);
    ioctl(fd, CDATA_CLEAR, &i);
    close(fd);
}
