#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "cdata_ioctl.h"

int main(void)
{
    int i;
    int fd;
    pid_t child;

    child = fork();

    fd = open("/dev/cdata", O_RDWR);

    /**
     * Parent process
     */
    if (child)
	goto parent;

child:
    ioctl(fd, IOCTL_NAME, "child");
    goto done;

parent:
    while (1)
        write(fd, "hello", 5);
    goto done;

done:
    close(fd);
    return 0;
}
