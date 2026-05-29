#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    uint8_t hdr[2];
    // Payload length 1, 2 (dynamic); Masking key (dynamic), Payload data (masked, dynamic)
    uint8_t data[];
} wcio_ws_frame;

// Continuation frame is omitted, as it is equal to 0
typedef enum _opcode
{
    TEXT_FRAME = 0x1,
    BINARY_FRAME = 0x2,
    CLOSE_CONNECTION = 0x8,
    PING = 0x9,
    PONG = 0xA
} wcio_header_opcode;

wcio_ws_frame *wcio_build_ws_frame(uint8_t is_final, wcio_header_opcode opcode, uint8_t *payload, size_t payload_size, size_t *resulting_frame_size);
uint8_t *wcio_ws_frame_to_bytes(wcio_ws_frame *frame, size_t frame_size);

#endif
