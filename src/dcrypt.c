#include "dcrypt.h"
#include "log.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string.h>

int aes_crypt(char* dest, char* value, size_t value_len, size_t* value_res_len, const char* auth_code, const char* iv, int enc) {
	EVP_CIPHER_CTX* ctx;
	char key[32];
	int len;

	if (SHA256((unsigned char*)auth_code, strlen(auth_code), (unsigned char*)key) == 0) {
		dlog(LOG_ERROR, "Failed to hash auth code");
		return 1;
	}

	if (!(ctx = EVP_CIPHER_CTX_new())) {
		dlog(LOG_ERROR, "Failed to init EVP");
		return 2;
	}

	if (EVP_CipherInit(ctx, EVP_aes_256_cbc(), (unsigned char*)key, (unsigned char*)iv, enc) != 1) {
		dlog(LOG_ERROR, "Failed to init EVP crypt");
		EVP_CIPHER_CTX_free(ctx);
		return 3;
	}

	if (EVP_CipherUpdate(ctx, (unsigned char*)dest, &len, (unsigned char*)value, value_len) != 1) {
		dlog(LOG_ERROR, "Failed to update EVP crypt");
		EVP_CIPHER_CTX_free(ctx);
		return 4;
	}
	*value_res_len = len;

	if (EVP_CipherFinal(ctx, (unsigned char*)(dest + *value_res_len), &len) != 1) {
		dlog(LOG_ERROR, "Failed to finish EVP crypt");
		EVP_CIPHER_CTX_free(ctx);
		return 5;
	}
	*value_res_len += len;

	EVP_CIPHER_CTX_free(ctx);

	return 0;
}
