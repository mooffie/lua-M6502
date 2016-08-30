#include <stdlib.h>

#include "utils.h"


uint8_t
read_byte(M6502 * mpu, uint16_t addr)
{
    M6502_Callback reader = M6502_getCallback(mpu, read, addr);
    if (reader)
        return reader(mpu, addr, -1);
    else
        return mpu->memory[addr];
}

void
write_byte(M6502 * mpu, uint16_t addr, uint8_t data)
{
    M6502_Callback writer = M6502_getCallback(mpu, write, addr);
    if (writer)
        writer(mpu, addr, data);
    else
        mpu->memory[addr] = data;
}

void
pushw(M6502 * mpu, uint16_t w)
{
    mpu->memory[mpu->registers->s-- + 0x100] = w >> 8;
    mpu->memory[mpu->registers->s-- + 0x100] = w & 0xff;
}

uint16_t
popw(M6502 * mpu)
{
    return mpu->memory[++mpu->registers->s + 0x100]
        | (mpu->memory[++mpu->registers->s + 0x100] << 8);
}

void
pushb(M6502 * mpu, uint8_t b)
{
    mpu->memory[mpu->registers->s-- + 0x100] = b;
}

uint8_t
popb(M6502 * mpu)
{
    return mpu->memory[++mpu->registers->s + 0x100];
}

int
default_BRK_handler(M6502 * mpu, uint16_t address, uint8_t data)
{
    char buffer[64];
    M6502_dump(mpu, buffer);
    printf("\nBRK instruction reached. Exiting.\n%s\n", buffer);
    exit(0);
}
