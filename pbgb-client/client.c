#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#define PORT 5000
#define MAXLINE 1000

#define LCD_WIDTH 160
#define LCD_HEIGHT 144
#define PIXEL_COUNT (LCD_WIDTH * LCD_HEIGHT)


int save_lcd_bmp(uint16_t fb[144][160]) {
	/* Should be enough to record up to 828 days worth of frames, apparently. */
	static uint32_t file_num = 0;
	char file_name[36];
	FILE *f;
	int ret = -1;

	snprintf(file_name, 36, "%.16s_%i.bmp", "test", file_num);

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


int main(int argc, char *argv[]) {
	char *ip_str;
	switch (argc) {
		case 2:
			/* Apply file name to rom_file_name
			 * Set save_file_name to NULL */
			ip_str = argv[1];
			break;
		default:
			fprintf(stderr, "error: invalid number of arguments\n");
			return EXIT_FAILURE;
	}

	SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;


	if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "error initializing sdl: %s\n", SDL_GetError());
        return 1;
    }


	window = SDL_CreateWindow("PBGB Client", LCD_WIDTH * 4, LCD_HEIGHT * 4, 0);

    if (window == NULL) {
        fprintf(stderr, "sdl error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

	renderer = SDL_CreateRenderer(window, 0);
    if (renderer == NULL) {
        fprintf(stderr, "sdl error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

	texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_XRGB1555,
        SDL_TEXTUREACCESS_STREAMING,
        LCD_WIDTH, LCD_HEIGHT
    );

    if (texture == NULL) {
        fprintf(stderr, "sdl error could not create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

	SDL_SetRenderLogicalPresentation(renderer, LCD_WIDTH, LCD_HEIGHT, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

	char buffer[PIXEL_COUNT*2]; // uint16_t pixel values is double the size of char
	char *message = "PBGB";
	int sockfd;
	struct sockaddr_in servaddr;

	// clear servaddr
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_addr.s_addr = inet_addr(ip_str);
	servaddr.sin_port = htons(PORT);
	servaddr.sin_family = AF_INET;

	// create datagram socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// connect to server
	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		printf("\n Error : Connect Failed \n");
		exit(0);
	}

	// request to send datagram
	// no need to specify server address in sendto
	// connect stores the peers IP and port
	sendto(sockfd, message, MAXLINE, 0, (struct sockaddr*)NULL, sizeof(servaddr));


    SDL_Event e;
	bool quit = 0;

	while (!quit) {
		while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }

		recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL);
		uint16_t fb[144][160];
		memcpy(fb, buffer, 160 * 144 * sizeof(uint16_t));
		// save_lcd_bmp(fb);

        if (!SDL_UpdateTexture(
            texture,
            NULL, // NULL means update the entire texture
            &buffer, // Pointer to the raw pixel data
            LCD_WIDTH * 2 // Pitch: bytes per row. 160 pixels * 2 bytes/pixel = 320 bytes
        )) {
            fprintf(stderr, "SDL_UpdateTexture failed: %s\n", SDL_GetError());
            continue;
        }

        // 5b. Clear the renderer, copy texture to renderer, and present
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        // Render the small 160x144 texture to fill the larger window
        SDL_RenderTexture(renderer, texture, NULL, NULL);

        SDL_RenderPresent(renderer);
    }

	// close the descriptor
	close(sockfd);
}