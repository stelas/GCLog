#include "logger.h"

void log_open(const char *name) {
	openlog(name, LOG_CONS | LOG_PID, LOG_LOCAL3);
}

void log_close() {
	closelog();
}

void log_inform(const char *msg) {
	syslog(LOG_NOTICE, "%s", msg);
}

void log_warn(const char *msg) {
	syslog(LOG_WARNING, "[WARNING] %s", msg);
}

void log_exclaim(const char *msg) {
	syslog(LOG_ERR, "[ERROR] %s", msg);
}

void log_debug(const char *msg) {
	syslog(LOG_DEBUG, "[DEBUG] %s", msg);
}
