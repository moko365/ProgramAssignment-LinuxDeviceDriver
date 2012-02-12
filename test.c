/*
 * Filename: test.c
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

int main(void)
{
    int fd;

    fd = open("/dev/cdata", O_RDWR);
    write(fd, "123", 3);
    close(fd);
}
