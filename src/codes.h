#ifndef CODES_H
#define CODES_H

#include "keys.h"

#define DURATION_LEN 30

int gen_codes(otp_key* keys);

unsigned long get_counter_val();

#endif
