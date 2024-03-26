#ifndef DEFINES_H
#define DEFINES_H

#define ARRAY_SIZE(x)   ((sizeof(x)) / sizeof(*(x)))
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

/* Bit to bit mask. */
#define BIT2MASK(bitpos) (1 << (bitpos))
/* Set bit at bitpos in value. */
#define BITSET(value, bitpos) ((value) |= BIT2MASK(bitpos))
/* Clear bit at bitpos in value. */
#define BITCLR(value, bitpos) ((value) &= ~BIT2MASK(bitpos))
/* Test bit at bitpos in value. */
#define BITVAL(value, bitpos) (((value) & BIT2MASK(bitpos)) != 0)

/* Reinterpret val as a two's complement signed integer of x bits. */
#define SIGNED_X_BITS(x, val) (((struct {signed int i : x;}){.i = val}).i)

#endif
