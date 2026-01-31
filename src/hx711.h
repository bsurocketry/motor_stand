
#ifndef _HX711

#define _HX711

/* moving this adc over to a seperate 
 * header so there can be some seperation
 * from the new one...
 */

#define PIN_DT  23
#define PIN_SCK 24

#define GAIN_32  2
#define GAIN_64  3
#define GAIN_128 1

/* max value we can return */
#define ADC_MAX 0x7fffff

void init_adc();

void reset_adc();

long next_value(int new_gain);

void close_adc();



#endif
