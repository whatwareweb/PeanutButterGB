#include "netlcd.h"

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int init_netlcd(struct lcd_server server) {
	int listenfd, len;

	struct sockaddr_in servaddr = server.servaddr;
	struct sockaddr_in cliaddr = server.cliaddr;

	bzero(&servaddr, sizeof(servaddr));
}