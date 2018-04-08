#include "tcpcli.h"

int tcp_connect(const char *host, const char *port) {
	int sock = -1;
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;		// AF_INET, AF_INET6 or AF_UNSPEC
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, port, &hints, &res) == 0) {
		if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) != -1)
			if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
				close(sock);
				sock = -1;
			}
		freeaddrinfo(res);
	}

	return sock;
}

void tcp_close(int sock) {
	close(sock);
}

int tcp_send(int sock, const char *str) {
	return send(sock, str, strlen(str), 0);
}

int tcp_receive(int sock, char *buffer, unsigned int size) {
	ssize_t n;

	if ((n = recv(sock, buffer, size - 1, 0)) >= 0)
		buffer[n] = '\0';		// Terminate string.

	return n;
}
