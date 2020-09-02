#include "codes.h"
#include "dotp/dotp.h"
#include <time.h>

#define CODE_LEN 6

unsigned long counter_val = 0;
unsigned long count_changes = 0;

unsigned long get_counter_val() {
	return time(0) / DURATION_LEN;
}

static int has_counter_changed() {
	unsigned long new_counter_val = get_counter_val();
	if (counter_val != new_counter_val) {
		counter_val = new_counter_val;
		return 1;
	}
	return 0;
}

int gen_codes(otp_key* keys) {
	otp_key* it;

	if (!has_counter_changed()) {
		return 0;
	}
	count_changes++;
	if (count_changes >= 3) {
		count_changes = counter_val = 0;
		return 1;
	}

	for (it = keys; it != 0; it = it->next) {
		if (dotp_totp_value(it->curr_code, it->key, it->key_size,
				DURATION_LEN, CODE_LEN)) {
			return -1;
		}
	}
	return 0;
}
