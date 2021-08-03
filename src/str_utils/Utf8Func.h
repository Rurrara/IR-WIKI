#include <stdint.h>
#include <stdlib.h>

uint8_t* StrToUtf8(uint8_t* dst, uint8_t* src);
uint8_t* strToLowerUtf8(uint8_t* ptr);
uint8_t* UnicodeToUtf8(uint8_t* pos, uint32_t val);