#include "netlcd.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int init_netlcd(struct lcd_server server) {
	memset(&server.servaddr, 0, sizeof(server.servaddr));

	// create udp socket
	server.listenfd = socket(AF_INET, SOCK_DGRAM, 0);
	server.servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server.servaddr.sin_port = htons(PORT);
	server.servaddr.sin_family = AF_INET;

	if (bind(server.listenfd, (struct sockaddr*)&server.servaddr, sizeof(server.servaddr)) == -1) {
		perror("error: could not bind to port");
		return -1;
	}

	char buffer[5];

	printf("info: waiting to recieve handshake from display client\n");

	server.len = sizeof(server.cliaddr);
	recvfrom(server.listenfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&server.cliaddr, &server.len);

	if (strcmp(buffer, "PBGB") != 0) {
		fprintf(stderr, "error: could not initialize server context: handshake invalid");
	}

	return 0;
}