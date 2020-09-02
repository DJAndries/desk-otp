#ifndef DCRYPT_H
#define DCRYPT_H

#include <stddef.h>
#define IV_SZ 16

int aes_crypt(char* dest, char* value, size_t value_len, size_t* value_res_len, const char* auth_code, const char* iv, int enc);

#endif
