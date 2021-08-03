#include <stdio.h>
#include <stdint.h>
#include <string.h>

int op_preced(const uint8_t c);
int op_left_assoc(const uint8_t c);
int shunting_yard(const uint8_t *input, uint8_t *output);
int ParsExpression(const uint8_t* input, uint8_t* output);
