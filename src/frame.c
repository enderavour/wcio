#include "include/frame.h"

#ifdef __APPLE__

#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)

#endif

wcio_ws_frame *wcio_build_ws_frame(uint8_t is_final, wcio_header_opcode opcode, uint8_t *payload, size_t payload_size, size_t *resulting_frame_size)
{
    if (payload == NULL)
        payload_size = 0;

    size_t alloc_len = payload_size;
    if (payload_size > 125 && payload_size <= 65535)
        alloc_len += 2;
    else if (payload_size > 65535)
        alloc_len += 8;

    // Masking key, mandatory for client
    alloc_len += 4;

    wcio_ws_frame *frame = (wcio_ws_frame*)calloc(1, sizeof(*frame) + alloc_len);

    // Setting FIN if is_final is set, and encoding opcode
    frame->hdr[0] = ((is_final & 1) << 7) | (opcode & 0x0F);

    // Mask key (mandatory for client) and payload length
    frame->hdr[1] = 0x80;

    uint8_t *p = frame->data;

    if (payload_size <= 125)
        frame->hdr[1] |= payload_size;
    else if (payload_size <= 65535)
    {
        frame->hdr[1] |= 126;
        *(uint16_t*)p = htons(payload_size);
        p += 2;
    }
    else
    {
        frame->hdr[1] |= 127;
        *(uint64_t*)p = htobe64(payload_size);
        p += 8;
    }

    uint8_t mask_key[4];
    for (int32_t i = 0; i < 4; ++i)
        mask_key[i] = rand() & 0xFF;

    memcpy(p, mask_key, 4);
    p += 4;

    for (int32_t i = 0; i < payload_size; i++)
        p[i] = payload[i] ^ mask_key[i % 4];

    *resulting_frame_size = alloc_len + 2;

    return frame;
}

uint8_t *wcio_ws_frame_to_bytes(wcio_ws_frame *frame, size_t frame_size)
{
    uint8_t *bytes = (uint8_t*)malloc(frame_size);
    memcpy(bytes, frame->hdr, 2);
    memcpy(bytes + 2, frame->data, frame_size - 2);
    free(frame);
    return bytes;
}
