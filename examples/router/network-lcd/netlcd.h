#ifndef NETLCD_H
#define NETLCD_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 5000

struct lcd_server {
	struct sockaddr_in servaddr, cliaddr;
};

int init_netlcd(struct lcd_server server);

#endif