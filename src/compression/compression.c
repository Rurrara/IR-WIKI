#include "compression.h"

uint32_t compress_uint32(uint32_t in, uint8_t *out) {
	
	uint32_t len = 0;
	while (in >= 0x80) {
		out[len++] = (in & 0x7f);
		in = in >> 7;
	}
	out[len++] = (in & 0x7f) + 0x80;
	return len;
}

uint32_t decompress_uint32(uint8_t *in, uint32_t *out) {

	*out = 0;
	uint32_t len = 0;
	while (*in < 0x80) {
		*out += (*(in++) << 7*len++);
	}
	*out += ((*in & 0x7f) << 7*len++);
	return len;
}
