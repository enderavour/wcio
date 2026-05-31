#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// Size of buffer which receives the handshake while upgrading the connection.
#define WCIO_HANDSHAKE_RECV_BUFFER_SIZE 230
// The maximum size of payload before splitting it and sending by chunks.
#define WCIO_FRAGMENT_SIZE 16384

#endif
