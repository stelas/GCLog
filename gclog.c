#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "logger.h"
#include "ini.h"
#include "diygeiger.h"
#include "gqgeiger.h"
#include "tcpcli.h"

#define GCLOG_VERSION	"0.2.4"
#define GCLOG_BUILD	"2018-04-14"
#define BUF_SIZE	1000
#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif

enum EGeiger { SIM, DIY, GQ };
static const char *GeigerNames[] = { "Geiger simulator", "DIY/MyGeiger/NET-IO Geiger Kit", "GQ GMC Geiger Counter" };
static volatile bool Running = true;

int div_round_closest(int n, int d) {
	return (n < 0) ^ (d < 0) ? (n - d/2) / d : (n + d/2) / d;
}

char * string_copy(const char *src) {
	char *dst = calloc(strlen(src) + 1, sizeof(char));
	strcpy(dst, src);

	return dst;
}

bool string_isset(char *str) {
	return str != NULL && strlen(str) > 0;
}

void try_free(char *buf) {
	if (buf != NULL)
		free(buf);
}

void signal_handler(int sig) {
	switch (sig) {
		case SIGTERM:
		case SIGINT:
		case SIGQUIT:
		case SIGHUP:
			Running = false;
		default:
			break;
	}
}

int geiger_open(enum EGeiger type, const char *device) {
	int fd = -1;
	time_t t = time(NULL);
	struct tm *tm = gmtime(&t);

	if (type == GQ) {
		if ((fd = gq_open(device)) != -1) {
			gq_set_date(fd, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
			gq_set_time(fd, tm->tm_hour,        tm->tm_min,     tm->tm_sec);
		}
	}
	else if (type == DIY) {
		fd = diy_open(device);
	}
	else if (type == SIM) {
		fd = STDIN_FILENO;
	}

	return fd;
}

void geiger_close(enum EGeiger type, int device) {
	if (type == GQ)
		gq_close(device);
	else if (type == DIY)
		diy_close(device);
}

int geiger_get_cpm(enum EGeiger type, int device) {
	if (type == GQ)
		return gq_get_cpm(device);
	else if (type == DIY)
		return diy_get_cpm(device);
	else if (type == SIM)
		return 12;	// Guaranteed to be random. :-)

	return 0;
}

bool send_gmcmap(const char *user, const char *device, int cpm) {
	char *req;
	int sock;
	char buf[BUF_SIZE] = { 0 };

	asprintf(&req, "GET /log2.asp?AID=%s&GID=%s&CPM=%d HTTP/1.1\r\nHost: www.gmcmap.com\r\n\r\n", user, device, cpm);

	if ((sock = tcp_connect("www.gmcmap.com", "80")) != -1) {
		tcp_send(sock, req);
		tcp_receive(sock, buf, BUF_SIZE);
		tcp_close(sock);
	}

	free(req);

	return sock != -1 && strstr(buf, "OK.") != NULL;
}

bool send_netc(const char *id, int cpm) {
	char *req;
	int sock;
	char buf[BUF_SIZE] = { 0 };

	asprintf(&req, "GET /push.php?id=%s&v=w32_1.1.3.1085&c=%d HTTP/1.1\r\nHost: radiation.netc.com\r\n\r\n", id, cpm);

	if ((sock = tcp_connect("radiation.netc.com", "80")) != -1) {
		tcp_send(sock, req);
		tcp_receive(sock, buf, BUF_SIZE);
		tcp_close(sock);
	}

	free(req);

	return sock != -1 && strstr(buf, "Ok.") != NULL;
}

bool send_radmon(const char *user, const char *pass, int cpm, const struct tm *tm) {
	char ch[22], *req;
	int sock;
	char buf[BUF_SIZE] = { 0 };

	strftime(ch, 22, "%Y-%m-%d%%20%H:%M:%S", tm);
	asprintf(&req, "GET /radmon.php?user=%s&password=%s&function=submit&datetime=%s&value=%d&unit=CPM HTTP/1.1\r\nHost: www.radmon.org\r\n\r\n", user, pass, ch, cpm);

	if ((sock = tcp_connect("www.radmon.org", "80")) != -1) {
		tcp_send(sock, req);
		tcp_receive(sock, buf, BUF_SIZE);
		tcp_close(sock);
	}

	free(req);

	return sock != -1 && strstr(buf, "Incorrect login.") == NULL;
}

bool send_safecast(const char *key, unsigned int dev, int cpm, const struct tm *tm, float lat, float lng, const char *loc) {
	char ch[21], *pld, *req;
	int sock;
	char buf[BUF_SIZE] = { 0 };

	strftime(ch, 21, "%Y-%m-%dT%H:%M:%SZ", tm);
	asprintf(&pld, "{\"latitude\":\"%.4f\",\"longitude\":\"%.4f\",\"location_name\":\"%s\",\"device_id\":\"%u\",\"captured_at\":\"%s\",\"value\":\"%d\",\"unit\":\"cpm\"}", lat, lng, loc, dev, ch, cpm);
	asprintf(&req, "POST /measurements.json?api_key=%s HTTP/1.1\r\nHost: api.safecast.org\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", key, strlen(pld), pld);

	if ((sock = tcp_connect("api.safecast.org", "80")) != -1) {
		tcp_send(sock, req);
		tcp_receive(sock, buf, BUF_SIZE);
		tcp_close(sock);
	}

	free(pld);
	free(req);

	return sock != -1 && strstr(buf, "201 Created") != NULL;
}

void print_usage() {
	printf("   ___    Geiger Counter LOGger daemon\n");
	printf("   \\_/    Version %s (Build %s)\n", GCLOG_VERSION, GCLOG_BUILD);
	printf(".--,O.--,\n");
	printf(" \\/   \\/  Copyright (C) 2014-18 Steffen Lange, gclog@stelas.de\n\n");
	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it\n");
	printf("under certain conditions. See the file COPYING for details.\n\n");
	printf("Usage: gclog -c <file> [-d]\n");
	printf("  -c <file>  load configuration from file\n");
	printf("  -d         activate verbose mode\n\n");
}

int main(int argc, char *argv[]) {
	bool debug = false;
	unsigned int interval = 60;
	enum EGeiger device_type = SIM;
	char *device_port = NULL;
	float latitude = 0.0, longitude = 0.0;
	char *location = NULL;
	char *netc_id = NULL;
	char *radmon_user = NULL, *radmon_pass = NULL;
	char *safecast_key = NULL;
	unsigned int safecast_device = 0;
	char *gmcmap_user = NULL, *gmcmap_device = NULL;

	print_usage();

	pid_t pid;

	if ((pid = fork()) == -1) {
		fprintf(stderr, "%s: fork() failed.\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		// Luke, I'm your father.
		exit(EXIT_SUCCESS);
	}

	if (setsid() == -1) {
		fprintf(stderr, "%s: setsid() failed.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGHUP, signal_handler);

	log_open("gclog");

	int opt;
	struct map_t *ini;
	char *val;

	while ((opt = getopt(argc, argv, "c:d")) != -1) {
		switch (opt) {
			case 'd':
				debug = true;
				break;

			case 'c':
				ini = load_ini(optarg);
				if ((val = map_get(ini, "interval")) != NULL)
					interval = MAX(atoi(val), 1);
				if ((val = map_get(ini, "device.type")) != NULL) {
					if (strcmp(val, "sim") == 0)
						device_type = SIM;
					else if (strcmp(val, "diy") == 0)
						device_type = DIY;
					else if (strcmp(val, "gq") == 0)
						device_type = GQ;
				}
				if ((val = map_get(ini, "device.port")) != NULL)
					device_port = string_copy(val);
				if ((val = map_get(ini, "latitude")) != NULL)
					latitude = MIN(MAX(atof(val), -90.0), 90.0);
				if ((val = map_get(ini, "longitude")) != NULL)
					longitude = MIN(MAX(atof(val), -180.0), 180.0);
				if ((val = map_get(ini, "location")) != NULL)
					location = string_copy(val);
				if ((val = map_get(ini, "netc.id")) != NULL)
					netc_id = string_copy(val);
				if ((val = map_get(ini, "radmon.user")) != NULL)
					radmon_user = string_copy(val);
				if ((val = map_get(ini, "radmon.pass")) != NULL)
					radmon_pass = string_copy(val);
				if ((val = map_get(ini, "safecast.key")) != NULL)
					safecast_key = string_copy(val);
				if ((val = map_get(ini, "safecast.device")) != NULL)
					safecast_device = atoi(val);
				if ((val = map_get(ini, "gmcmap.user")) != NULL)
					gmcmap_user = string_copy(val);
				if ((val = map_get(ini, "gmcmap.device")) != NULL)
					gmcmap_device = string_copy(val);
				map_free(ini);
				break;

			default:
				break;
		}
	}

	if (debug)
		printf("Configuration:\n\t\t%s on %s,\n\t\tLocation: %s (%f, %f)\n\t\tnetc.com: %s,\n\t\tradmon.org: %s / %s,\n\t\tsafecast.org: %s / Device ID %u,\n\t\tgmcmap.com: %s / Device ID %s,\n\t\t%us interval\n\n", GeigerNames[device_type], device_port, location, latitude, longitude, netc_id, radmon_user, radmon_pass, safecast_key, safecast_device, gmcmap_user, gmcmap_device, interval);

	int fd = -1;

	if (string_isset(device_port) && ((fd = geiger_open(device_type, device_port)) != -1)) {
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		time_t last = time(NULL);
		int cpm, sum = 0, count = 0;

		while (Running) {
			if ((cpm = geiger_get_cpm(device_type, fd)) > 0) {
				sum += cpm;
				count++;
			}

			if (difftime(time(NULL), last) >= interval) {
				if (count > 0) {
					struct tm *tm = gmtime(&last);
					cpm = div_round_closest(sum, count);

					if (debug) {
						char *cpmstr;
						asprintf(&cpmstr, "CPM: %d (= %d/%d), Timestamp: %s", cpm, sum, count, asctime(tm));
						log_inform(cpmstr);
						free(cpmstr);
					}

					if (string_isset(netc_id))
						if(!send_netc(netc_id, cpm))
							log_warn("Upload to netc.com failed.");
					if (string_isset(radmon_user) && string_isset(radmon_pass))
						if (!send_radmon(radmon_user, radmon_pass, cpm, tm))
							log_warn("Upload to radmon.org failed.");
					if (string_isset(safecast_key) && string_isset(location))
						if(!send_safecast(safecast_key, safecast_device, cpm, tm, latitude, longitude, location))
							log_warn("Upload to safecast.org failed.");
					if (string_isset(gmcmap_user) && string_isset(gmcmap_device))
						if(!send_gmcmap(gmcmap_user, gmcmap_device, cpm))
							log_warn("Upload to gmcmap.com failed.");

					time(&last);
					sum = count = 0;
				}
				else
					log_exclaim("Reading ZERO value from Geiger tube.");
			}

			// Let the cpu breathe.
			sleep(1);
		}

		geiger_close(device_type, fd);
	}
	else
		perror("FATAL ERROR: Could not access Geiger counter");

	try_free(device_port);
	try_free(location);
	try_free(netc_id);
	try_free(radmon_user);
	try_free(radmon_pass);
	try_free(safecast_key);
	try_free(gmcmap_user);
	try_free(gmcmap_device);

	log_close();

	return fd == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
