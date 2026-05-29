#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>

uint8_t *wcio_base64encode(const uint8_t *data_in, size_t input_length, size_t *output_length);
uint8_t *wcio_base64decode(const uint8_t *data_in, size_t input_length, size_t *output_length);
uint8_t *wcio_gen_secure_key();


#endif
