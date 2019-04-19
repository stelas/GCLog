#include "diygeiger.h"

int diy_open(const char *device, speed_t baud) {
	int fd = -1;
	struct termios tio;

	memset(&tio, 0, sizeof(struct termios));
	tio.c_cflag = CS8 | CREAD | CLOCAL;		// 8n1
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 5;

	if ((fd = open(device, O_RDWR)) != -1) {
		if (cfsetspeed(&tio, baud) == 0)		// usually 9600 baud
			if (tcsetattr(fd, TCSANOW, &tio) == 0)
				// Return file descriptor,
				return fd;

		// ... otherwise close device.
		close(fd);
		fd = -1;
	}

	return fd;
}

void diy_close(int device) {
	close(device);
}

int diy_get_cpm(int device) {
	char buf[13] = { 0 };

	// Arduino sends 10-digit decimal number (unsigned long) plus tailing "\r\n".
	diy_read(device, buf, 12);

	return atoi(buf);
}

int diy_read(int device, char *buf, unsigned int len) {
	unsigned int n;
	ssize_t rcvd = 0;
	char *inp = &buf[0];

	for (n = 0; n < len; n++) {
		rcvd += read(device, inp, 1);
		inp = &buf[rcvd];
		if (rcvd >= len)
			break;
	}

	return rcvd;
}
