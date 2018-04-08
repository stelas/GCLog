#ifndef _TCPCLI_H_
#define _TCPCLI_H_

#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

int tcp_connect(const char *host, const char *port);
void tcp_close(int sock);
int tcp_send(int sock, const char *str);
int tcp_receive(int sock, char *buffer, unsigned int size);

#endif
