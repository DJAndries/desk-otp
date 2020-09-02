#include "keys.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "dotp/dotp.h"
#include "dcrypt.h"

#define LOCAL_KEYS "desk-otp-keys.conf"
#define ETC_KEYS "/etc/desk-otp-keys.conf"
#define TMP_KEYS "/tmp/desk-otp-keys.conf"
#define LINE_SZ 512

static void gen_iv(char* iv) {
	size_t i;
	for (i = 0; i < 16; i++) {
		iv[i] = rand() & 0xFF;
	}
}

static FILE* load_file(const char* local_file, const char* etc_file, char* src_filename) {
	FILE* f = fopen(local_file, "r");
	if (!f) {
		f = fopen(etc_file, "r");
		if (!f) {
			return 0;
		}
		if (src_filename) strcpy(src_filename, etc_file);
		return f;
	}
	if (src_filename) strcpy(src_filename, local_file);
	return f;
}

static int parse_key_and_label(char* line, char* label, char* key, size_t* key_size) {
	char* end_of_key;
	char enc_key[LINE_SZ];
	size_t label_len;

	if ((end_of_key = strchr(line, ':')) == 0) {
		dlog(LOG_ERROR, "Could not load key of encrypted key");
		return 1;
	}

	memcpy(enc_key, line, end_of_key - line);
	enc_key[end_of_key - line] = 0;

	if (dotp_b32_decode(key, key_size, enc_key)) {
		dlog(LOG_ERROR, "Could not decode key of encrypted key");
		return 2;
	}
	if (*(end_of_key + 1) == 0) {
		dlog(LOG_ERROR, "No label available for key");
		return 3;
	}
	strcpy(label, end_of_key + 1);
	label_len = strlen(label);
	if (label[label_len - 1] == '\n') {
		label[label_len - 1] = 0;
	}
	return 0;
}

static int load_encrypted(char* line, char* label, char* key, size_t* key_size, char* iv, const char* auth_code) {
	char encr_key[LINE_SZ];
	char enc_iv[LINE_SZ];
	char* end_of_iv;
	size_t iv_sz;
	size_t encr_key_sz;

	if ((end_of_iv = strchr(line + 2, '#')) == 0) {
		dlog(LOG_ERROR, "Could not load IV of encrypted key");
		return 1;
	}

	memcpy(enc_iv, line + 2, end_of_iv - (line + 2));
	enc_iv[end_of_iv - (line + 2)] = 0;

	if (dotp_b32_decode(iv, &iv_sz, enc_iv)) {
		dlog(LOG_ERROR, "Could not decode IV of encrypted key");
		return 2;
	}

	if (iv_sz != IV_SZ) {
		dlog(LOG_ERROR, "IV length is not 16 bytes");
		return 3;
	}

	if (parse_key_and_label(end_of_iv + 1, label, encr_key, &encr_key_sz)) {
		return 4;
	}
	
	if (aes_crypt(key, encr_key, encr_key_sz, key_size, auth_code, iv, 0)) {
		return 5;
	}
	return 0;
}

static int create_key(otp_key** okey, char* label, char* key, size_t key_size) {
	if ((*okey = (otp_key*)malloc(sizeof(otp_key))) == 0) {
		dlog(LOG_ERROR, "Failed to alloc key struct");
		return 1;
	}
	memset(*okey, 0, sizeof(otp_key));
	if (((*okey)->key = (char*)malloc(sizeof(char) * key_size)) == 0) {
		free(*okey);
		dlog(LOG_ERROR, "Failed to alloc key array");
		return 2;
	}
	memcpy((*okey)->key, key, key_size);
	(*okey)->key_size = key_size;
	if (((*okey)->label = (char*)malloc(sizeof(char) * (strlen(label) + 1))) == 0) {
		free((*okey)->key);
		free(*okey);
		dlog(LOG_ERROR, "Failed to alloc key label");
		return 3;
	}
	strcpy((*okey)->label, label);
	return 0;
}

static int overwrite_file(const char* dest_name, const char* src_name) {
	FILE* src;
	FILE* dest;
	char line[LINE_SZ];
	if ((dest = fopen(dest_name, "w")) == 0) {
		dlog(LOG_ERROR, "Failed to open config for overwriting");
		return 1;
	}
	if ((src = fopen(src_name, "r")) == 0) {
		dlog(LOG_ERROR, "Failed to open temp config for reading");
		fclose(dest);
		return 2;
	}
	while(fgets(line, LINE_SZ, src)) {
		if (fputs(line, dest) == -1) {
			dlog(LOG_ERROR, "Failed to write to config");
			fclose(dest);
			fclose(src);
		}
	}

	fclose(src);
	fclose(dest);
	return 0;
}

int encrypt_keys(const char* auth_code, otp_key* keys) {
	FILE* f;
	FILE* wf;
	char line[LINE_SZ];
	char enc_iv[LINE_SZ];
	char encr_key[LINE_SZ];
	char enc_key[LINE_SZ];
	char src_filename[64];
	char iv[16];
	size_t encr_size;
	otp_key* curr = keys;

	srand(time(0));

	if ((f = load_file(LOCAL_KEYS, ETC_KEYS, src_filename)) == 0) {
		dlog(LOG_ERROR, "Failed to open config");
		return 1;
	}
	if ((wf = fopen(TMP_KEYS, "w")) == 0) {
		dlog(LOG_ERROR, "Failed to open config temp file");
		fclose(f);
		return 1;
	}
	while(fgets(line, LINE_SZ, f) && curr) {
		if (line[0] == 0 || line[0] == '\n') continue;

		if (line[0] == 'e') {
			fprintf(wf, "%s", line);
		} else {
			gen_iv(iv);
			dotp_b32_encode(enc_iv, iv, 16);
			if (aes_crypt(encr_key, curr->key, curr->key_size, &encr_size, auth_code, iv, 1)) {
				fclose(f);
				fclose(wf);
				remove(TMP_KEYS);
				return 2;
			}
			dotp_b32_encode(enc_key, encr_key, encr_size);
			fprintf(wf, "e#%s#%s:%s\n", enc_iv, enc_key, curr->label);
		}
		curr = curr->next;
	}
	fclose(f);
	fclose(wf);
	if (overwrite_file(src_filename, TMP_KEYS)) {
		remove(TMP_KEYS);
		return 3;
	}
	remove(TMP_KEYS);
	return 0;
}

int load_keys(const char* auth_code, otp_key** keys) {
	FILE* f;
	char line[LINE_SZ];
	char key[LINE_SZ];
	char label[LINE_SZ];
	char iv[LINE_SZ];
	size_t key_size;
	otp_key* curr;
	otp_key* tail;
	*keys = tail = 0;

	if ((f = load_file(LOCAL_KEYS, ETC_KEYS, 0)) == 0) {
		dlog(LOG_ERROR, "Failed to open config");
		return 1;
	}

	while(fgets(line, LINE_SZ, f)) {
		if (line[0] == 0 || line[0] == '\n') continue;

		if (line[0] == 'e') {
			if (load_encrypted(line, label, key, &key_size, iv, auth_code)) {
				free_keys(*keys);
				fclose(f);
				return 2;
			}
		} else {
			if (parse_key_and_label(line, label, key, &key_size)) {
				free_keys(*keys);
				fclose(f);
				return 3;
			}
		}

		if (create_key(&curr, label, key, key_size)) {
			free_keys(*keys);
			fclose(f);
			return 4;
		}
		if (!tail) {
			*keys = curr;
		} else {
			tail->next = curr;
		}
		tail = curr;
	}

	fclose(f);
	return 0;
}

void free_keys(otp_key* keys) {
	otp_key* it;
	otp_key* next;
	for (it = keys; it; it = next) {
		next = it->next;
		free(it->key);
		free(it->label);
		free(it);
	}
}
