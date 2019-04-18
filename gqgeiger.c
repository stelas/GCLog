#include "gqgeiger.h"

int gq_open(const char *device) {
	int fd = -1;
	struct termios tio;

	memset(&tio, 0, sizeof(struct termios));
	tio.c_cflag = CS8 | CREAD | CLOCAL;		// 8n1
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 5;

	if ((fd = open(device, O_RDWR)) != -1) {
		if (cfsetspeed(&tio, B57600) == 0)		// 57600 baud
			if (tcsetattr(fd, TCSANOW, &tio) == 0)
				// Turn off heartbeat,
				if (gq_set_heartbeat_off(fd))
					return fd;

		// ... otherwise close device.
		close(fd);
		fd = -1;
	}

	return fd;
}

void gq_close(int device) {
	close(device);
}

int gq_get_cpm(int device) {
	const char cmd[] = "<GETCPM>>";
	char buf[3] = { 0 };

	if (gq_write(device, cmd) == (ssize_t) strlen(cmd))
		gq_read(device, buf, 2);

	return buf[0] * 256 + buf[1];
}

bool gq_set_heartbeat_off(int device) {
	const char cmd[] = "<HEARTBEAT0>>";

	if (gq_write(device, cmd) == (ssize_t) strlen(cmd))
		return gq_flush(device);

	return false;
}

bool gq_set_date(int device, int year, int month, int day) {
	char y[] = "<SETDATEYY >>";
	char m[] = "<SETDATEMM >>";
	char d[] = "<SETDATEDD >>";

	y[10] = year - 2000;
	m[10] = month;
	d[10] = day;

	return gq_write_ack(device, y) && gq_write_ack(device, m) && gq_write_ack(device, d);
}

bool gq_set_time(int device, int hour, int minute, int second) {
	char h[] = "<SETTIMEHH >>";
	char m[] = "<SETTIMEMM >>";
	char s[] = "<SETTIMESS >>";

	h[10] = hour;
	m[10] = minute;
	s[10] = second;

	return gq_write_ack(device, h) && gq_write_ack(device, m) && gq_write_ack(device, s);
}

bool gq_write_ack(int device, const char *cmd) {
	char ch;

	if (gq_write(device, cmd) == (ssize_t) strlen(cmd))
		if (gq_read(device, &ch, 1) == 1 && ch == '\xAA')
			return true;

	return false;
}

int gq_write(int device, const char *cmd) {
	return write(device, cmd, strlen(cmd));
}

int gq_read(int device, char *buf, unsigned int len) {
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

bool gq_flush(int device) {
	int i;
	char ch;

	for (i = 0; i < 10; i++) {
		if (read(device, &ch, 1) == 0)
			return true;
	}

	return false;
}
