#include <stdint.h>
#include <stdbool.h>

bool v4l2_decompress(uint8_t *buffer, int buf_size, uint8_t *output, int width, int height);
