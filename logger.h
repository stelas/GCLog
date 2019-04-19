#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <syslog.h>

void log_open(const char *name);
void log_close();
void log_inform(const char *msg);
void log_warn(const char *msg);
void log_exclaim(const char *msg);
void log_debug(const char *msg);

#endif
