#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/joystick.h>

#include "minigb_apu/minigb_apu.h"
#include "fileutils/fileutils.h"

#include "btnmap.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

uint8_t audio_read(uint16_t addr);
void audio_write(uint16_t addr, uint8_t val);


#include "network-lcd/netlcd.h"

#include "../../peanut_gb.h"

#define AUDIO_BUFFER_SIZE 256


static struct minigb_apu_ctx apu;

static struct lcd_server lcd;

struct priv_t {
	/* Pointer to allocated memory holding GB file */
	uint8_t *rom;
	/* Pointer to allocated memory holding save file */
	uint8_t *cart_ram;
	/* Size of the cart_ram in bytes */
	size_t save_size;
	/* Pointer to boot ROM binary if available */
	uint8_t *bootrom;

	/* Colour palette for each BG, OBJ0, and OBJ1 */
	uint16_t selected_palette[3][4];
	uint16_t fb[LCD_HEIGHT][LCD_WIDTH];
};

/**
 * Returns a byte from the ROM file at the given address
 */
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr) {
	const struct priv_t * const p = gb->direct.priv;
	return p->rom[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address
 */
uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr) {
	const struct priv_t * const p = gb->direct.priv;
	return p->cart_ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address
 */
void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val){
	const struct priv_t * const p = gb->direct.priv;
	p->cart_ram[addr] = val;
}

uint8_t gb_bootrom_read(struct gb_s *gb, const uint_fast16_t addr) {
	const struct priv_t * const p = gb->direct.priv;
	return p->bootrom[addr];
}

uint8_t audio_read(uint16_t addr) {
	return minigb_apu_audio_read(&apu, addr);
}

void audio_write(uint16_t addr, uint8_t val) {
	minigb_apu_audio_write(&apu, addr, val);
}

/*
void audio_callback(void *ptr, uint8_t *data, int len) {
	minigb_apu_audio_callback(&apu, (void *)data);
}
*/

void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	audio_sample_t* stream = (audio_sample_t*)pOutput;
	minigb_apu_audio_callback(&apu, (void *)stream, frameCount);
}

void read_cart_ram_file(const char *save_file_name, uint8_t **dest, const size_t len) {
	FILE *f;

	/* If save file not required, return */
	if (len == 0) {
		*dest = NULL;
		return;
	}

	/* Allocate enough memory to hold save file */
	if ((*dest = malloc(len)) == NULL) {
		perror("error: could not load save file");
		exit(EXIT_FAILURE);
	}

	f = fopen(save_file_name, "rb");

	/* It doesn't matter if the save file doesn't exist. We initialize the
	 * save memory allocated above. The save file will be created on exit. */
	if (f == NULL) {
		memset(*dest, 0, len);
		return;
	}

	/* Read save file to allocated memory */
	fread(*dest, sizeof(uint8_t), len, f);
	fclose(f);
}

void write_cart_ram_file(const char *save_file_name, uint8_t **dest, const size_t len) {
	FILE *f;

	if (len == 0 || *dest == NULL)
		return;

	if ((f = fopen(save_file_name, "wb")) == NULL) {
		perror("error: could not write to save file");
		return;
	}

	/* Record save file */
	fwrite(*dest, sizeof(uint8_t), len, f);
	fclose(f);

	return;
}

/**
 * Handles an error reported by the emulator. The emulator context may be used
 * to better understand why the error given in gb_err was reported.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr) {
	const char* gb_err_str[GB_INVALID_MAX] = {
		"UNKNOWN",
		"INVALID OPCODE",
		"INVALID READ",
		"INVALID WRITE",
		""
	};
	struct priv_t *priv = gb->direct.priv;
	char error_msg[256];
	char location[64] = "";
	uint8_t instr_byte;

	/* Record save file */
	write_cart_ram_file("recovery.sav", &priv->cart_ram, priv->save_size);

	if (addr >= 0x4000 && addr < 0x8000) {
		uint32_t rom_addr;
		rom_addr = (uint32_t)addr * (uint32_t)gb->selected_rom_bank;
		snprintf(location, sizeof(location),
			     " (bank %d mode %d, file offset %u)",
			     gb->selected_rom_bank, gb->cart_mode_select, rom_addr);
	}

	instr_byte = __gb_read(gb, addr);

	snprintf(error_msg, sizeof(error_msg),
		     "Error: %s at 0x%04X%s with instruction %02X.\n"
		     "Cart RAM saved to recovery.sav\n"
		     "Exiting.\n",
	      gb_err_str[gb_err], addr, location, instr_byte);

	fprintf(stderr, "error in emulation:\n %s\n", error_msg);

	/* Free memory and then exit */
	free(priv->cart_ram);
	free(priv->rom);
	exit(EXIT_FAILURE);
}

/**
 * Draws scanline into framebuffer.
 */
void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
	struct priv_t *priv = gb->direct.priv;

	for(unsigned int x = 0; x < LCD_WIDTH; x++) {
		priv->fb[line][x] = priv->selected_palette
				    [(pixels[x] & LCD_PALETTE_ALL) >> 4]
				    [pixels[x] & 3];
	}
}

/**
 * Saves the LCD screen as a 15-bit BMP file
 */
int save_lcd_bmp(struct gb_s* gb, uint16_t fb[LCD_HEIGHT][LCD_WIDTH]) {
	/* Should be enough to record up to 828 days worth of frames, apparently. */
	static uint_fast32_t file_num = 0;
	char file_name[32];
	char title_str[16];
	FILE *f;
	int ret = -1;

	snprintf(file_name, 32, "%.16s_%010ld.bmp", gb_get_rom_name(gb, title_str), file_num);

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
		fwrite(fb, sizeof(uint16_t), LCD_HEIGHT * LCD_WIDTH, f);
		ret = fclose(f);

		file_num++;
	}

	return ret;
}


// timing functions - must initialize the tick counter first

static struct timespec start_time;

int initialize_get_ticks() {
	if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) {
		return 1;
	}

	return 0;
}

uint64_t get_ticks_ms() {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    uint64_t diff_sec = current_time.tv_sec - start_time.tv_sec;
    uint64_t diff_nsec = current_time.tv_nsec - start_time.tv_nsec;

    // Adjust for negative nanosecond difference
    if (diff_nsec < 0) {
        diff_sec--;
        diff_nsec += 1000000000;
    }

    return ((diff_sec * 1000) + (diff_nsec / 1000000));
}

void delay_ms(uint64_t ms) {
	uint64_t initialTicks = get_ticks_ms();
	while (get_ticks_ms() - initialTicks < ms) {
		;
	}
	return;
}


int main(int argc, char *argv[]) {
	struct gb_s gb;

	struct priv_t priv = {
		.rom = NULL,
		.cart_ram = NULL
	};

	const double target_speed_ms = 1000.0 / VERTICAL_SYNC;
	double speed_compensation = 0.0;
	uint_fast32_t new_ticks, old_ticks;
	enum gb_init_error_e gb_ret;
	unsigned int fast_mode = 1;
	unsigned int fast_mode_timer = 1;
	/* Record save file every 60 seconds */
	int save_timer = 60;
	char *rom_file_name = NULL;
	char *save_file_name = NULL;
	int ret = EXIT_SUCCESS;

	init_netlcd(lcd);

	switch (argc) {
		case 2:
			/* Apply file name to rom_file_name
			 * Set save_file_name to NULL */
			rom_file_name = argv[1];
			break;
		case 3:
			/* Apply file name to rom_file_name
			 * Apply save name to save_file_name */
			rom_file_name = argv[1];
			save_file_name = argv[2];
			break;
		default:
			fprintf(stderr, "error: invalid number of arguments\n");
			ret = EXIT_FAILURE;
			goto out;
	}

	/* Copy input ROM file to allocated memory */
	if ((priv.rom = load_file(rom_file_name)) == NULL) {
		char errormsg[256];
		snprintf(errormsg, sizeof(errormsg), "error: failure loading rom file: %s", rom_file_name);
		perror(errormsg);
		ret = EXIT_FAILURE;
		goto out;
	}


	/* If no save file is specified, copy save file (with specific name) to
	 * allocated memory */
	if(save_file_name == NULL) {
		char *str_replace;
		const char extension[] = ".sav";

		/* Allocate enough space for the ROM file name, for the "sav"
		 * extension and for the null terminator */
		save_file_name = malloc(strlen(rom_file_name) + strlen(extension) + 1);

		if(save_file_name == NULL) {
			perror("error: could not create space for save file name");
			ret = EXIT_FAILURE;
			goto out;
		}

		/* Copy the ROM file name to allocated space */
		strcpy(save_file_name, rom_file_name);

		/* If the file name does not have a dot, or the only dot is at
		 * the start of the file name, set the pointer to begin
		 * replacing the string to the end of the file name, otherwise
		 * set it to the dot. */
		if((str_replace = strrchr(save_file_name, '.')) == NULL || str_replace == save_file_name)
			str_replace = save_file_name + strlen(save_file_name);

		/* Copy extension to string including terminating null byte. */
		for(unsigned int i = 0; i <= strlen(extension); i++)
			*(str_replace++) = extension[i];
	}


	/* Initialize emulator context */
	gb_ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, &priv);

	switch(gb_ret) {
	case GB_INIT_NO_ERROR:
		break;

	case GB_INIT_CARTRIDGE_UNSUPPORTED:
		fprintf(stderr, "error: unsupported cartridge\n");
		ret = EXIT_FAILURE;
		goto out;

	case GB_INIT_INVALID_CHECKSUM:
		fprintf(stderr, "error: invalid ROM, checksum failure\n");
		ret = EXIT_FAILURE;
		goto out;

	default:
		fprintf(stderr, "error: emulator ctx could not be initialized (unknown)\n");
		ret = EXIT_FAILURE;
		goto out;
	}


	/* Copy dmg_boot.bin boot ROM file to allocated memory */
	if((priv.bootrom = load_file("dmg_boot.bin")) == NULL) {
		printf("info: no dmg_boot.bin file found; disabling boot ROM\n");
	} else {
		printf("info: boot ROM enabled\n");
		gb_set_bootrom(&gb, gb_bootrom_read);
		gb_reset(&gb);
	}


	/* Load Savefile */
	if(gb_get_save_size_s(&gb, &priv.save_size) < 0) {
		fprintf(stderr, "error: unable to get save size \n");
		ret = EXIT_FAILURE;
		goto out;
	}

	/* Only attempt to load a save file if the ROM actually supports saves */
	if(priv.save_size > 0)
		read_cart_ram_file(save_file_name, &priv.cart_ram, priv.save_size);



	/* Set the RTC of the game cartridge. Only used by games that support it */
	{
		time_t rawtime;
		time(&rawtime);
#ifdef _POSIX_C_SOURCE
		struct tm timeinfo;
		localtime_r(&rawtime, &timeinfo);
#else
		struct tm *timeinfo;
		timeinfo = localtime(&rawtime);
#endif

		/* You could potentially force the game to allow the player to
		 * reset the time by setting the RTC to invalid values.
		 *
		 * Using memset(&gb->cart_rtc, 0xFF, sizeof(gb->cart_rtc)) for
		 * example causes Pokemon Gold/Silver to say "TIME NOT SET",
		 * allowing the player to set the time without having some dumb
		 * password.
		 *
		 * The memset has to be done directly to gb->cart_rtc because
		 * gb_set_rtc() processes the input values, which may cause
		 * games to not detect invalid values.
		 */

		/* Set RTC. Only games that specify support for RTC will use
		 * these values. */
#ifdef _POSIX_C_SOURCE
		gb_set_rtc(&gb, &timeinfo);
#else
		gb_set_rtc(&gb, timeinfo);
#endif
	}

	/* 14 for "PeanutButter: " and a maximum of 16 for the title. */
	char title_str[30] = "PeanutButter: ";
	gb_get_rom_name(&gb, title_str + 14);
	printf("%s\n", title_str);

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format    = ma_format_s16;       // Set to ma_format_unknown to use the device's native format.
    config.playback.channels  = 2;                   // Set to 0 to use the device's native channel count.
    config.sampleRate         = AUDIO_SAMPLE_RATE;   // Set to 0 to use the device's native sample rate.
    config.dataCallback       = audio_data_callback;       // This function will be called when miniaudio needs more data.
	config.periodSizeInFrames = AUDIO_BUFFER_SIZE;

	ma_device device;
	if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        fprintf(stderr, "error: failed to initialize audio");
		ret = EXIT_FAILURE;
		goto out;
    }

	ma_device_start(&device);
	minigb_apu_audio_init(&apu);

	gb_init_lcd(&gb, &lcd_draw_line);

	int js_fd;
	struct js_event jse;
	// set up linux joystick 0
    js_fd = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
    if (js_fd < 0) {
        perror("error: could not open joystick device");
        return 1;
    }


	/* Main loop */
	while(true) {
		int delay;
		static double rtc_timer = 0;
		static unsigned int selected_palette = 3;
		static unsigned int dump_bmp = 0;

		/* Calculate the time taken to draw frame, then later add a
		* delay to cap at 60 fps. */
		old_ticks = get_ticks_ms();

		// reading the joystick and apply to emulator context
		if (read(js_fd, &jse, sizeof(jse)) > 0) {
			switch (jse.type & ~JS_EVENT_INIT) {
				case JS_EVENT_BUTTON:
					switch (jse.number) {
						case A_BTN:
							gb.direct.joypad = (jse.value) ? ~JOYPAD_A : gb.direct.joypad | JOYPAD_A;
							break;
						case B_BTN:
							gb.direct.joypad = (jse.value) ? ~JOYPAD_B : gb.direct.joypad | JOYPAD_B;
							break;
						case SEL_BTN:
							gb.direct.joypad = (jse.value) ? ~JOYPAD_SELECT : gb.direct.joypad | JOYPAD_SELECT;
							break;
						case STA_BTN:
							gb.direct.joypad = (jse.value) ? ~JOYPAD_START : gb.direct.joypad | JOYPAD_START;
							break;
					}
					break;
				case JS_EVENT_AXIS:
					switch (jse.number) {
						case 0:
							gb.direct.joypad = (jse.value < -JS_DEADZONE) ? ~JOYPAD_LEFT : gb.direct.joypad | JOYPAD_LEFT;
							gb.direct.joypad = (jse.value > JS_DEADZONE) ? ~JOYPAD_RIGHT : gb.direct.joypad | JOYPAD_RIGHT;
							break;
						case 1:
							gb.direct.joypad = (jse.value < -JS_DEADZONE) ? ~JOYPAD_UP : gb.direct.joypad | JOYPAD_UP;
							gb.direct.joypad = (jse.value > JS_DEADZONE) ? ~JOYPAD_DOWN : gb.direct.joypad | JOYPAD_DOWN;
							break;
					}
					break;
        	}
    	}

		// All SDL_PollEvent and input handling has been removed.

		/* Execute CPU cycles until the screen has to be redrawn. */
		gb_run_frame(&gb);

		/* Skip frames during fast mode. */
		if(fast_mode_timer > 1) {
			fast_mode_timer--;
			/* We continue here since the rest of the logic in the
			* loop is for drawing the screen and delaying. */
			continue;
		}

		fast_mode_timer = fast_mode;

		/* TODO - reimplement this as non-SDL
		// Copy frame buffer to SDL screen.
		SDL_UpdateTexture(texture, NULL, &priv.fb, LCD_WIDTH * sizeof(uint16_t));
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		*/


		if(dump_bmp) {
			if(save_lcd_bmp(&gb, priv.fb) != 0) {
				fprintf(stderr, "error: failed to dump frames\n");
				dump_bmp = 0;
				printf("info: stopped dumping frames\n");
			}
		}

		/* Use a delay that will draw the screen at a rate of 59.7275 Hz. */
		new_ticks = get_ticks_ms();

		/* Since we can only delay for a maximum resolution of 1ms, we
		* can accumulate the error and compensate for the delay
		* accuracy when the delay compensation surpasses 1ms. */
		speed_compensation += target_speed_ms - (new_ticks - old_ticks);

		/* We cast the delay compensation value to an integer, since it
		* is the type used by SDL_Delay. This is where delay accuracy
		* is lost. */
		delay = (int)(speed_compensation);

		/* We then subtract the actual delay value by the requested
		* delay value. */
		speed_compensation -= delay;

		/* Only run delay logic if required. */
		if(delay > 0) {
			uint_fast32_t delay_ticks = get_ticks_ms();
			uint_fast32_t after_delay_ticks;

			/* Tick the internal RTC when 1 second has passed. */
			rtc_timer += delay;

			if(rtc_timer >= 1000) {
				/* If 60 seconds has passed, record save file.
				* We do this because the blarrg audio library
				* used contains asserts that will abort the
				* program without save.
				* TODO: Remove all workarounds due to faulty
				* external libraries. */
				--save_timer;

				if(!save_timer) {
					write_cart_ram_file(save_file_name, &priv.cart_ram, priv.save_size);
					save_timer = 60;
				}
			}

			/* This will delay for at least the number of
			* milliseconds requested, so we have to compensate for
			* error here too. */
			usleep(delay * 1000);

			after_delay_ticks = get_ticks_ms();
			speed_compensation += (double)delay - (int)(after_delay_ticks - delay_ticks);
		}
	}

	ma_device_uninit(&device);

out:
	free(priv.rom);
	free(priv.cart_ram);
	return ret;
}
