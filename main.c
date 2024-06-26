#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "atmega328p.h"
#include "cpu.h"

int main(int argc, char *argv[])
{
    struct atmega328p mcu;

    atmega328p_init(&mcu);

    int actual = fread(mcu.flash, 2, 1000, stdin);

    for (int i = 0; i < actual + 20; ++i) {
        cpu_cycle(&mcu.cpu);
    }

    return 0;
}
