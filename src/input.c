#include "input.h"
#include "log.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>

#define CONSUMER_NAME "desk-otp"
#define MAX_PINS 9

#define BOUNCE_TIME_MS 250

struct gpiod_chip* chip = 0;
struct gpiod_line* pins[MAX_PINS];
size_t gpio_pin_count = 0;
struct timespec last_event_time = { 0, 0 };

static long timespec_sub_ms(struct timespec* a, struct timespec* b) {
	long ams = (a->tv_sec * 1000) + (a->tv_nsec / 1000000);
	long bms = (b->tv_sec * 1000) + (b->tv_nsec / 1000000);
	return ams - bms;
}

int init_input(char* current_seq, size_t* seq_len) {
	char numbuf[8];
	unsigned gpio_index;
	char* comma;
	size_t len;
	char default_gpio[] = DEFAULT_GPIO_PINS;
	char* gpio_pins = getenv("GPIO_PINS");

	memset(current_seq, 0, MAX_SEQ);
	*seq_len = 0;

	gpio_pin_count = 0;
	if (!gpio_pins) gpio_pins = default_gpio;

	if ((chip = gpiod_chip_open("/dev/gpiochip0")) == 0) {
		dlog(LOG_ERROR, "Failed to open GPIO chip");
		return 1;
	}
	
	while (gpio_pins[0] != 0 && gpio_pin_count < MAX_PINS) {
		comma = strchr(gpio_pins, ',');
		if (comma) {
			len = comma - gpio_pins;
		} else {
			len = strlen(gpio_pins);
		}
		if (len >= 8) {
			dlog(LOG_ERROR, "GPIO pin number is too long");
			return 2;
		}
		memcpy(numbuf, gpio_pins, len);
		numbuf[len] = 0;
		if (sscanf(numbuf, "%u", &gpio_index) != 1) {
			dlog(LOG_ERROR, "Failed to parse GPIO pin number");
			return 3;
		}
		if ((pins[gpio_pin_count] = gpiod_chip_get_line(chip, gpio_index)) == 0) {
			dlog(LOG_ERROR, "Failed to get GPIO pin");
			return 4;
		}
		if (gpiod_line_request_rising_edge_events(pins[gpio_pin_count], CONSUMER_NAME)) {
			dlog(LOG_ERROR, "Failed to reserve GPIO pin");
			return 5;
		}

		gpio_pin_count++;
		gpio_pins += comma ? (len + 1) : len;
	}
	return 0;
}

int input_get_seq(char* current_seq, size_t* seq_len) {
	size_t i;
	long time_diff;
	char numstr[16];
	struct gpiod_line_event ev;
	struct timespec wait_timeout = { 0, 800000 };

	for (i = 0; i < gpio_pin_count; i++) {
		if (gpiod_line_event_wait(pins[i], &wait_timeout) == 0) continue;

		if (gpiod_line_event_read(pins[i], &ev)) continue;

		sprintf(numstr, "%u", i + 1);

		time_diff = timespec_sub_ms(&ev.ts, &last_event_time);
		if (time_diff >= 0 && time_diff < BOUNCE_TIME_MS) {
			dlog(LOG_DEBUG, "Possible button bounce index %u, ignoring", i + 1);
			continue;
		}

		sprintf(numstr, "%u", i + 1);
		dlog(LOG_DEBUG, "Pressed index %u", i + 1);

		last_event_time = ev.ts;

		if (*seq_len >= 16) {
			dlog(LOG_INFO, "Max sequence length exceeded, ignoring press");
			continue;
		};

		current_seq[(*seq_len)++] = numstr[0];
		current_seq[*seq_len] = 0;
	}

	if (last_event_time.tv_sec && (time(0) - last_event_time.tv_sec) >= 2) {
		last_event_time.tv_sec = 0;
		last_event_time.tv_nsec = 0;
		*seq_len = 0;
		dlog(LOG_DEBUG, "Sequence complete");
		return 0;
	}
	return 1;
}

void free_input() {
	size_t i;
	for (i = 0; i < gpio_pin_count; i++) {
		gpiod_line_release(pins[i]);
	}
	gpio_pin_count = 0;
	gpiod_chip_close(chip);
}

