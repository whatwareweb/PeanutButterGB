#ifndef NETLCD_H
#define NETLCD_H

#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 5000

struct lcd_server {
	struct sockaddr_in servaddr, cliaddr;
	int listenfd, len;
};

int init_netlcd(struct lcd_server server);

#endif