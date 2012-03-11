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
    char pix[4] = {0x00, 0xff, 0x00, 0xff};
    char *fb;

    i = 10000;
    fd = open("/dev/cdata", O_RDWR);
    //ioctl(fd, CDATA_CLEAR, &i);
    //while(1) {
    //	write(fd, pix, 4);
    //}
    fb = (char *)mmap(0, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
}
