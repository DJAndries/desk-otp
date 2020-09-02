#ifndef FB_H
#define FB_H

#include "keys.h"

int init_fb();

int draw_frame(otp_key* keys);

void free_fb();

#endif
