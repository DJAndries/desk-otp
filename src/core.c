#include <stdio.h>
#include <unistd.h>
#include "log.h"
#include "keys.h"
#include "input.h"
#include "fb.h"
#include "codes.h"
#include "fbmagic/fbmagic.h"

otp_key* keys = 0;
char current_seq[MAX_SEQ + 1];
size_t pending_seq_len;
int fb_lock = 0;

int exec_not_active(char* active) {

	if (input_get_seq(current_seq, &pending_seq_len) == 0) {
		dlog(LOG_INFO, "Auth attempt");
		if (load_keys(current_seq, &keys)) {
			return 0;
		}

		if (encrypt_keys(current_seq, keys)) {
			return 1;
		}

		fb_lock = fbmagic_lock_acquire(1);
		if (fb_lock == 0) {
			dlog(LOG_ERROR, "Failed to acquire lock");
			return 2;
		}

		*active = 1;
	}
	return 0;
}

int exec_active(char* active) {
	int result = gen_codes(keys);
	if (result == 1) {
		/* three counter changes occurred, finish otp screen */
		free_keys(keys);
		keys = 0;
		fbmagic_lock_release(fb_lock);
		fb_lock = 0;
		*active = 0;
		return 0;
	} else if (result == -1) {
		/* code gen failed */
		return 2;
	}

	if (draw_frame(keys)) {
		return 1;
	}
	return 0;
}

int main(int argc, char** argv) {
	int result;
	char active = 0;

	if (init_fb(argc >= 2 ? argv[1] : 0)) {
		return 1;
	}

	if (init_input(current_seq, &pending_seq_len)) {
		free_fb();
		return 3;
	}

	while(1) {
		if (!active) {
			result = exec_not_active(&active);
		} else {
			result = exec_active(&active);		
		}
		if (result) {
			if (keys) free_keys(keys);
			free_input();
			free_fb();
			return 4;
		}
		usleep(16000);
	}

	if (keys) free_keys(keys);
	free_input();
	free_fb();
	
	return 0;
}
