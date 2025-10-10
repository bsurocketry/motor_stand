/* gpio_compat.c - stub implementations when pigpio is not available */

#ifndef HAVE_PIGPIO
#include "gpio_compat.h"
#include <stdio.h>
#include <time.h>

int gpioInitialise(void) {
    /* no-op on non-pi; return success */
    fprintf(stderr, "gpioInitialise: pigpio not present; running in emulation mode\n");
    return 0;
}

void gpioTerminate(void) {
    /* no-op */
}

int gpioSetMode(int pin, int mode) {
    (void)pin; (void)mode; return 0;
}

void gpioWrite(int pin, int level) {
    (void)pin; (void)level; /* no-op */
}

int gpioRead(int pin) {
    (void)pin; return 0; /* always 0 */
}

void gpioDelay(unsigned int microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

#endif
