#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 5000
#define MAXLINE 1000


int save_lcd_bmp(uint16_t fb[144][160]) {
	/* Should be enough to record up to 828 days worth of frames, apparently. */
	static uint32_t file_num = 0;
	char file_name[36];
	FILE *f;
	int ret = -1;

	snprintf(file_name, 36, "%.16s_%010ld.bmp", "test", file_num);

	f = fopen(file_name, "wb");

	if (f != NULL) {
		const uint8_t bmp_hdr_rgb555[] = {
			0x42, 0x4d, 0x36, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xa0, 0x00,
			0x00, 0x00, 0x70, 0xff, 0xff, 0xff, 0x01, 0x00, 0x10, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};

		fwrite(bmp_hdr_rgb555, sizeof(uint8_t), sizeof(bmp_hdr_rgb555), f);
		fwrite(fb, sizeof(uint16_t), 144 * 160, f);
		ret = fclose(f);

		file_num++;
	}

	return ret;
}


int main() {
	char buffer[46080];
	char *message = "PBGB";
	int sockfd, n;
	struct sockaddr_in servaddr;

	// clear servaddr
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(PORT);
	servaddr.sin_family = AF_INET;

	// create datagram socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// connect to server
	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
		exit(0);
	}

	// request to send datagram
	// no need to specify server address in sendto
	// connect stores the peers IP and port
	sendto(sockfd, message, MAXLINE, 0, (struct sockaddr*)NULL, sizeof(servaddr));

	// waiting for response
	while (1){
	recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL);
	uint16_t fb[144][160];
	memcpy(fb, buffer, 160 * 144 * sizeof(uint16_t));
	save_lcd_bmp(fb);
	}

	// close the descriptor
	close(sockfd);
}