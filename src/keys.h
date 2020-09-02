#ifndef KEYS_H
#define KEYS_H

#include <stddef.h>

typedef struct otp_key otp_key;
struct otp_key {
	char* key;
	char* label;
	char curr_code[8];
	size_t key_size;
	otp_key* next;
};

int load_keys(const char* auth_code, otp_key** keys);

int encrypt_keys(const char* auth_code, otp_key* keys);

void free_keys(otp_key* keys);

#endif
