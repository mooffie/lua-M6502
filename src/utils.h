#ifndef M6502__UTILS_H
#define M6502__UTILS_H

#include "../lib/piumarta/lib6502.h"

/* Debugging aid. */
#if 0
#  define d_message(args) (printf args)
#else
#  define d_message(args)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

uint8_t read_byte(M6502 * mpu, uint16_t addr);
void write_byte(M6502 * mpu, uint16_t addr, uint8_t data);

void pushw(M6502 * mpu, uint16_t w);
uint16_t popw(M6502 * mpu);
void pushb(M6502 * mpu, uint8_t b);
uint8_t popb(M6502 * mpu);

int default_BRK_handler(M6502 * mpu, uint16_t address, uint8_t data);

#endif
