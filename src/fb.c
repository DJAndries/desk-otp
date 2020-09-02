#include "fb.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "fbmagic/fbmagic.h"
#include "log.h"
#include "codes.h"

#define TEXT_SCALE 1.6f

fbmagic_ctx* ctx;
fbmagic_image* bg;
fbmagic_font* font;

int init_fb(const char* device) {
	char* path_prefix = getenv("RESOURCE_PATH_PREFIX");
	char res_prefix[256];
	char filename[300];
	if (path_prefix == 0) {
		sprintf(res_prefix, "res/");
	} else { 
		sprintf(res_prefix, "%sres/", path_prefix);
	}

	if ((ctx = fbmagic_init(device ? device : "/dev/fb1")) == 0) {
		return 1;
	}

	sprintf(filename, "%s%s", res_prefix, "bg.bmp");
	if ((bg = fbmagic_load_bmp(ctx, filename)) == 0) {
		fbmagic_exit(ctx);
		return 2;
	}

	sprintf(filename, "%s%s", res_prefix, "gohufont-11.bdf");
	if ((font = fbmagic_load_bdf(filename)) == 0) {
		fbmagic_exit(ctx);
		fbmagic_free_image(bg);                 
		return 3;                                        
	}

	return 0;
}

void draw_clock() {
	char content[16];
	struct timeval tv;
	int progress_color = fbmagic_color_value(ctx, 0xFF, 0x66, 0x0);
	int text_color = fbmagic_color_value(ctx, 0x20, 0x20, 0x20);
	gettimeofday(&tv, 0);

	unsigned long time_ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	float time_left = 1.f - (float)(time_ms % (DURATION_LEN * 1000)) / (DURATION_LEN * 1000);

	sprintf(content, "%ds", (int)(time_left * DURATION_LEN));

	fbmagic_stroke(ctx, 20, 200, 280, 20, 1, progress_color);
	fbmagic_fill(ctx, 20, 200, (size_t)(280 * time_left), 20, progress_color);
	fbmagic_draw_text(ctx, font, 155, 204, content, text_color, TEXT_SCALE);
}

int draw_frame(otp_key* keys) {
	size_t x, y;
	char content[256];
	otp_key* it;
	int primary_color = fbmagic_color_value(ctx, 0x80, 0x0, 0x0);

	x = y = 20;

	fbmagic_draw_image_quick(ctx, 0, 0, bg);

	for (it = keys; it != 0 && y < 190; it = it->next) {
		sprintf(content, "%s: %s", it->label, it->curr_code);
		fbmagic_draw_text(ctx, font, x, y, content, primary_color, TEXT_SCALE);
		x = x == 20 ? 170 : 20;
		y += x == 20 ? 25 : 0;
	}

	draw_clock();

	fbmagic_flush(ctx);
	return 0;
}

void free_fb() {
	fbmagic_free_image(bg);
	free(font);
	fbmagic_exit(ctx);
}
