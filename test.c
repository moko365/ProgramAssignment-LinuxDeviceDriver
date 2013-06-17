#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    pid_t child;

    child = fork();

    fd = open("/dev/cdata", O_RDWR);
    ioctl(fd, IOCTL_INIT, NULL);
    close(fd);
}
