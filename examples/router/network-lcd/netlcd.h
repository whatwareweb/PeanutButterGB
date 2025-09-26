#ifndef NETLCD_H
#define NETLCD_H

#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 5000
#define MAXLINE 46080

typedef uint8_t bmp_DMG;

struct lcd_server {
	struct sockaddr_in servaddr, cliaddr;
	int listenfd, len;
};

int init_netlcd(struct lcd_server *server);

void send_frame_netlcd(struct lcd_server *server, uint16_t fb[144][160]);

#endif