#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

#define DEFAULT_GPIO_PINS "26,16,6,5"
#define MAX_SEQ 16

int init_input(char* current_seq, size_t* seq_len);

int input_get_seq(char* current_seq, size_t* seq_len);

void free_input();

#endif
