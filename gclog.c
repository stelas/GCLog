#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "logger.h"
#include "ini.h"
#include "diygeiger.h"
#include "gqgeiger.h"
#include "tcpcli.h"

#define GCLOG_VERSION	"0.2.5"
#define GCLOG_BUILD	"2019-04-19"
#define BUF_SIZE	1000
#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif

typedef enum { SIM, DIY, GQ } Geiger;
static const char *GeigerNames[] = { "Geiger simulator", "DIY/MyGeiger/NET-IO Geiger Kit", "GQ GMC Geiger Counter" };

typedef struct {
	unsigned int interval;
	Geiger device_type;
	char *device_port;
	speed_t device_baudrate;
	float latitude, longitude;
	char *location;
	char *netc_id;
	char *radmon_user, *radmon_pass;
	char *safecast_key;
	unsigned int safecast_device;
	char *gmcmap_user, *gmcmap_device;
} Settings;

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

speed_t baud_rate(unsigned int bps) {
	switch (bps) {
		case 1200:
			return B1200;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		default:
			return B0;	// Hang up.
	}
}

int geiger_open(Geiger type, const char *device, speed_t baud) {
	int fd = -1;
	time_t t = time(NULL);
	struct tm *tm = gmtime(&t);

	if (type == GQ) {
		if ((fd = gq_open(device, baud)) != -1) {
			gq_set_date(fd, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
			gq_set_time(fd, tm->tm_hour,        tm->tm_min,     tm->tm_sec);
		}
	}
	else if (type == DIY) {
		fd = diy_open(device, baud);
	}
	else if (type == SIM) {
		fd = STDIN_FILENO;
	}

	return fd;
}

void geiger_close(Geiger type, int device) {
	if (type == GQ)
		gq_close(device);
	else if (type == DIY)
		diy_close(device);
}

int geiger_get_cpm(Geiger type, int device) {
	if (type == GQ)
		return gq_get_cpm(device);
	else if (type == DIY)
		return diy_get_cpm(device);
	else if (type == SIM)
		return 12;	// Guaranteed to be random. ;-)

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
	printf(" \\/   \\/  Copyright (C) 2014-19 Steffen Lange, gclog@stelas.de\n\n");
	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it\n");
	printf("under certain conditions. See the file COPYING for details.\n\n");
	printf("Usage: gclog -c <file> [-vdh]\n");
	printf("  -h         display this help information\n\n");
	printf("  -c <file>  load configuration from file\n");
	printf("  -d         run in debug mode\n\n");
	printf("  -v         turn on verbose logging\n\n");
}

void init_settings(Settings *s) {
	s->interval = 60;
	s->device_type = SIM;
	s->device_port = NULL;
	s->device_baudrate = B0;
	s->latitude = 0.0;
	s->longitude = 0.0;
	s->location = NULL;
	s->netc_id = NULL;
	s->radmon_user = NULL;
	s->radmon_pass = NULL;
	s->safecast_key = NULL;
	s->safecast_device = 0;
	s->gmcmap_user = NULL;
	s->gmcmap_device = NULL;
}

void free_settings(Settings *s) {
	try_free(s->device_port);
	try_free(s->location);
	try_free(s->netc_id);
	try_free(s->radmon_user);
	try_free(s->radmon_pass);
	try_free(s->safecast_key);
	try_free(s->gmcmap_user);
	try_free(s->gmcmap_device);
}

int main(int argc, char *argv[]) {
	bool verbose = false, debug = false;
	Settings cfg;

	init_settings(&cfg);

	/* Parsing command-line options */
	int opt;
	struct map_t *ini;
	char *val;

	while ((opt = getopt(argc, argv, "c:vdh")) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				free_settings(&cfg);
				exit(EXIT_SUCCESS);
				break;

			case 'v':
				verbose = true;
				break;

			case 'd':
				debug = true;
				break;

			case 'c':
				ini = load_ini(optarg);
				free_settings(&cfg);
				if ((val = map_get(ini, "interval")) != NULL)
					cfg.interval = MAX(atoi(val), 1);
				if ((val = map_get(ini, "device.type")) != NULL) {
					if (strcmp(val, "sim") == 0)
						cfg.device_type = SIM;
					else if (strcmp(val, "diy") == 0)
						cfg.device_type = DIY;
					else if (strcmp(val, "gq") == 0)
						cfg.device_type = GQ;
				}
				if ((val = map_get(ini, "device.port")) != NULL)
					cfg.device_port = string_copy(val);
				if ((val = map_get(ini, "device.baudrate")) != NULL)
					cfg.device_baudrate = baud_rate(atoi(val));
				if ((val = map_get(ini, "latitude")) != NULL)
					cfg.latitude = MIN(MAX(atof(val), -90.0), 90.0);
				if ((val = map_get(ini, "longitude")) != NULL)
					cfg.longitude = MIN(MAX(atof(val), -180.0), 180.0);
				if ((val = map_get(ini, "location")) != NULL)
					cfg.location = string_copy(val);
				if ((val = map_get(ini, "netc.id")) != NULL)
					cfg.netc_id = string_copy(val);
				if ((val = map_get(ini, "radmon.user")) != NULL)
					cfg.radmon_user = string_copy(val);
				if ((val = map_get(ini, "radmon.pass")) != NULL)
					cfg.radmon_pass = string_copy(val);
				if ((val = map_get(ini, "safecast.key")) != NULL)
					cfg.safecast_key = string_copy(val);
				if ((val = map_get(ini, "safecast.device")) != NULL)
					cfg.safecast_device = atoi(val);
				if ((val = map_get(ini, "gmcmap.user")) != NULL)
					cfg.gmcmap_user = string_copy(val);
				if ((val = map_get(ini, "gmcmap.device")) != NULL)
					cfg.gmcmap_device = string_copy(val);
				map_free(ini);
				break;

			default:
				break;
		}
	}

	if (verbose)
		printf("Configuration:\n\t\t%s on %s @ %#07o,\n\t\tLocation: %s (%.4f, %.4f)\n\t\tnetc.com: %s,\n\t\tradmon.org: %s / %s,\n\t\tsafecast.org: %s / Device ID %u,\n\t\tgmcmap.com: %s / Device ID %s,\n\t\t%us interval\n\n", GeigerNames[cfg.device_type], cfg.device_port, cfg.device_baudrate, cfg.location, cfg.latitude, cfg.longitude, cfg.netc_id, cfg.radmon_user, cfg.radmon_pass, cfg.safecast_key, cfg.safecast_device, cfg.gmcmap_user, cfg.gmcmap_device, cfg.interval);

	/* Initializing device communication */
	int fd = -1;

	if ((fd = geiger_open(cfg.device_type, cfg.device_port, cfg.device_baudrate)) == -1) {
		perror("FATAL ERROR: Could not access Geiger counter");
		free_settings(&cfg);
		exit(EXIT_FAILURE);
	}

	/* Self-daemonizing */
	pid_t pid;

	if (!debug) {
		if ((pid = fork()) == -1) {
			fprintf(stderr, "%s: fork() failed.\n", argv[0]);
			geiger_close(cfg.device_type, fd);
			free_settings(&cfg);
			exit(EXIT_FAILURE);
		}
		if (pid > 0) {
			// Luke, I'm your father.
			geiger_close(cfg.device_type, fd);
			free_settings(&cfg);
			exit(EXIT_SUCCESS);
		}

		if (setsid() == -1) {
			fprintf(stderr, "%s: setsid() failed.\n", argv[0]);
			geiger_close(cfg.device_type, fd);
			free_settings(&cfg);
			exit(EXIT_FAILURE);
		}

		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGHUP, signal_handler);

	/* Main loop */
	time_t last = time(NULL);
	int cpm, sum = 0, count = 0;

	log_open("gclog");

	if (verbose)
		log_debug("Entering main loop...");

	while (Running) {
		if ((cpm = geiger_get_cpm(cfg.device_type, fd)) > 0) {
			sum += cpm;
			count++;
		}

		if (difftime(time(NULL), last) >= cfg.interval) {
			if (count > 0) {
				struct tm *tm = gmtime(&last);
				cpm = div_round_closest(sum, count);

				if (verbose) {
					char *cpmstr;
					asprintf(&cpmstr, "CPM: %d (= %d/%d), Timestamp: %s", cpm, sum, count, asctime(tm));
					log_inform(cpmstr);
					free(cpmstr);
				}

				if (string_isset(cfg.netc_id))
					if(!send_netc(cfg.netc_id, cpm))
						log_warn("Upload to netc.com failed.");
				if (string_isset(cfg.radmon_user) && string_isset(cfg.radmon_pass))
					if (!send_radmon(cfg.radmon_user, cfg.radmon_pass, cpm, tm))
						log_warn("Upload to radmon.org failed.");
				if (string_isset(cfg.safecast_key) && string_isset(cfg.location))
					if(!send_safecast(cfg.safecast_key, cfg.safecast_device, cpm, tm, cfg.latitude, cfg.longitude, cfg.location))
						log_warn("Upload to safecast.org failed.");
				if (string_isset(cfg.gmcmap_user) && string_isset(cfg.gmcmap_device))
					if(!send_gmcmap(cfg.gmcmap_user, cfg.gmcmap_device, cpm))
						log_warn("Upload to gmcmap.com failed.");

				time(&last);
				sum = count = 0;
			}
			else
				log_exclaim("Reading ZERO value from Geiger tube.");
		}

		// Let the CPU breathe.
		sleep(1);
	}

	if (verbose)
		log_debug("Main loop exited. Cleaning up...");

	geiger_close(cfg.device_type, fd);
	free_settings(&cfg);
	log_close();

	return EXIT_SUCCESS;
}
