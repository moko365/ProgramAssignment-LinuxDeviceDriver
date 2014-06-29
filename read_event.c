#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

/*
struct input_event {
    struct timeval time;
    __u16 type;
    __u16 code;
    __s32 value;
};
*/

#define FILE_PATH "/dev/input/event0"

int main()
{
	struct input_event event[8];
	int fd;

	printf("Reading %s\n", FILE_PATH);


	fd = open(FILE_PATH, O_RDWR);

	if(fd < 0)
	{
		printf("ERROR: %s can not open\n", FILE_PATH);
		exit(0);
	}

    read(fd, &event, sizeof(struct input_event)*2);

    /**
     * Input event codes:
     *    http://www.kernel.org/doc/Documentation/input/event-codes.txt
     *
     */
    switch (event[0].code) {       // type is EV_ABS
        case ABS_X:
            printf("X: %d\n", event[0].value);
            break;
        case ABS_Y:
            printf("Y: %d\n", event[0].value);
            break;
    }

	close(fd);
	return 0;
}

