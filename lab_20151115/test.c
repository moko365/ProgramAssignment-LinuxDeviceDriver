#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "cdata_ioctl.h"

int main()
{
   int fd;
   int i;
   pid_t child;

   fd = open("/dev/cdata", O_RDWR);
   if (fd == -1) {
      printf("can't open file [%d]\n", errno);
      exit(1);
   }

   for (i = 0; i < 32; i++)
      write(fd, "hello", 5);
   close(fd);
}
