#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
// Encode the data into Base64.
uint8_t *wcio_base64encode(const uint8_t *data_in, size_t input_length, size_t *output_length);
// Decode data from Base64.
uint8_t *wcio_base64decode(const uint8_t *data_in, size_t input_length, size_t *output_length);
// Generate Base64 key for Sec-WebSocket-Key.
uint8_t *wcio_gen_secure_key();

#endif
