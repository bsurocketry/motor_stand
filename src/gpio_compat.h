/* gpio_compat.h
 * Small compatibility layer to allow building on systems without pigpio.
 * When compiled with -DHAVE_PIGPIO the real pigpio headers are used.
 */

#ifndef GPIO_COMPAT_H
#define GPIO_COMPAT_H

#ifdef HAVE_PIGPIO
#include <pigpio.h>
#else
#include <stdint.h>

/* Minimal subset of pigpio-like API used by the project. These are
 * implemented as stubs for non-Raspberry Pi builds so the project
 * can compile and run (without hardware access).
 */

/* modes */
#define PI_INPUT 0
#define PI_OUTPUT 1

/* prototypes for the functions the code uses */
int gpioInitialise(void);
void gpioTerminate(void);
int gpioSetMode(int pin, int mode);
void gpioWrite(int pin, int level);
int gpioRead(int pin);
void gpioDelay(unsigned int microseconds);

#endif /* HAVE_PIGPIO */

#endif /* GPIO_COMPAT_H */
