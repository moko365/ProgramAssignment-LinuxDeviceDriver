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
	struct input_event event[32];
	int fd;
	int i;
	int n;

	fd = open("/dev/misc/cdata-ts", O_RDONLY);

	printf("Reading %s\n", FILE_PATH);

	fd = open(FILE_PATH, O_RDONLY);

	if(fd < 0)
	{
		printf("ERROR: %s can not open\n", FILE_PATH);
		exit(0);
	}

    setsid();

repeat:
    n = read(fd, event, sizeof(struct input_event)*32);

    /**
     * Input event codes:
     *    http://www.kernel.org/doc/Documentation/input/event-codes.txt
     *
     */
    for (i = 0; i < n/sizeof(struct input_event); i++) {
	if (event[i].type == EV_KEY &&
		event[i].code == BTN_TOUCH &&
		event[i].value == 0)
	    goto repeat;

        switch (event[i].code) {       // type is EV_ABS
            case ABS_X:
                printf("X: %d\n", event[i].value);
                break;
            case ABS_Y:
                printf("Y: %d\n", event[i].value);
                break;
        }
    }

    goto repeat;

    close(fd);

    return 0;
}

