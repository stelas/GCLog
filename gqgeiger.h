#ifndef _GQGEIGER_H_
#define _GQGEIGER_H_

#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 GQ Geiger Counter Communication Protocol: http://www.gqelectronicsllc.com/download/GQ-RFC1201.txt
*/

int gq_open(const char *device);
void gq_close(int device);
int gq_get_cpm(int device);
bool gq_set_heartbeat_off(int device);
bool gq_set_date(int device, int year, int month, int day);
bool gq_set_time(int device, int hour, int minute, int second);
bool gq_write_ack(int device, const char *cmd);
int gq_write(int device, const char *cmd);
int gq_read(int device, char *buf, unsigned int len);
bool gq_flush(int device);

#endif
