#ifndef _DIYGEIGER_H_
#define _DIYGEIGER_H_

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 Specification of GC10 Data I/O Interface: http://netiodev.com/GC10/GC10_UART_SPEC.pdf
*/

int diy_open(const char *device);
void diy_close(int device);
int diy_get_cpm(int device);
int diy_read(int device, char *buf, unsigned int len);

#endif
