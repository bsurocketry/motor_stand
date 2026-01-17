
#ifndef _ADS1115

#define _ADS1115
#include <stdlib.h>
#include <stdio.h>
#include <pigpio.h>

/* for compatability */
#define GAIN_32  0
#define GAIN_64  0
#define GAIN_128 0

/* minimal driver for the ADS1115 */

#define I2C_ADDR  0x48
#define I2C_0     0
#define I2C_1     1
#define I2C_FLAGS 0

#define CONV_REG 0
#define CONF_REG 1
#define LOTH_REG 2
#define HITH_REG 3

/* yeah yeah... */
static int i2c_handle;

/* data source multiplexer */
/*          POS  NEG  */
#define MUX_AIN0_AIN1  0
#define MUX_AIN0_AIN3  1
#define MUX_AIN1_AIN3  2
#define MUX_AIN2_AIN3  3
#define MUX_AIN0_GND   4
#define MUX_AIN1_GND   5
#define MUX_AIN2_GND   6
#define MUX_AIN3_GND   7

/* gain settings */
#define GAIN_6_144V 0 /* +/- 6.114V */
#define GAIN_4_096V 1 /* +/- 4.096V */
#define GAIN_2_048V 2 /* +/- 2.048V */
#define GAIN_1_024V 3 /* +/- 1.024V */
#define GAIN_0_512V 4 /* +/- 0.512V */
#define GAIN_0_256V 5 /* +/- 0.256V */

/* adc mode */
#define MODE_CONTIN 0
#define MODE_SINGLE 1

/* data rate (samples per second) */
#define SPS_8   0
#define SPS_16  1
#define SPS_32  2
#define SPS_64  3
#define SPS_128 4
#define SPS_250 5
#define SPS_475 6
#define SPS_860 7

/* comparator mode */
#define COMP_MODE_DEFAULT 0
#define COMP_MODE_WINDOW  1

/* toggle comparator polarity */
#define COMP_POL_ACT_LOW  0
#define COMP_POL_ACT_HIGH 1

/* toggle comparator latching */
#define COMP_LATCH_NO  0
#define COMP_LATCH_YES 1

/* comparator queue/disable setting */
#define COMP_QUE_SINGLE 0
#define COMP_QUE_DOUBLE 1
#define COMP_QUE_TRIPLE 2
#define COMP_QUE_OFF    3

struct ads1115_settings {
   int input_multiplexer:3;
   int gain_amplifier:3;
   int operating_mode:1;
   int data_rate:3;
   int comparator_mode:1;
   int comparator_polarity:1;
   int latching_comparator:1;
   int comparator_queue_disable:2;
};

static struct ads1115_settings config;

#define PIN_RDY 17

void init_adc();

void reset_adc();

long next_value(int new_gain);

void close_adc();

#endif
