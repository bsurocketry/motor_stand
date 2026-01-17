
#include "ads1115.h"
#include <unistd.h>

void init_adc() {

   int retval = 0;
   if ( (retval = gpioInitialise()) < 0) {
      printf("failed to initialize pigpio: %d\n",retval);
      exit(1);
   }

   if ( (i2c_handle = i2cOpen(I2C_1,I2C_ADDR,I2C_FLAGS)) < 0) {
      printf("failed to open i2c bus: %d\n",i2c_handle);
      exit(1);
   }

   if ( (retval = gpioSetMode(PIN_RDY, PI_INPUT)) < 0) {
      printf("failed to set PIN_RDY(%d) to input: %d\n",PIN_RDY,retval);
      exit(1);
   }

   /* configure everything */
   config.input_multiplexer        = MUX_AIN0_AIN1;
   config.gain_amplifier           = GAIN_0_256V;
   config.operating_mode           = MODE_CONTIN;
   config.data_rate                = SPS_860;
   config.comparator_mode          = COMP_MODE_DEFAULT;
   config.comparator_polarity      = COMP_POL_ACT_HIGH;
   config.latching_comparator      = COMP_LATCH_YES;
   config.comparator_queue_disable = COMP_QUE_SINGLE;

   /* set up the adc */
   reset_adc();
}

static int is_ready() {
   int res = gpioRead(PIN_RDY);
   return res == config.comparator_polarity;
}

/* it turns out the adc uses big endian
 * instead, at least for the config register */
#define SWAP_BITS(x)             \
   do {                          \
      unsigned short __temp = 0; \
      __temp |= (x&0xff00)>>8;   \
      __temp |= (x&0x00ff)<<8;   \
      x = __temp;                \
   } while (0)

static void set_lothresh(short val) {
   SWAP_BITS(val);
   i2cWriteWordData(i2c_handle,LOTH_REG,val);
}

static void set_hithresh(short val) {
   SWAP_BITS(val);
   i2cWriteWordData(i2c_handle,HITH_REG,val);
}

static void write_config() {
   unsigned short val = 0;
   val |= (config.input_multiplexer&0x7)<<12;       /* bits[14:12] */
   val |= (config.gain_amplifier&0x7)<<9;           /* bits[11:9]  */
   val |= (config.operating_mode&0x1)<<8;           /* bits[8]     */
   val |= (config.data_rate&0x7)<<5;                /* bits[7:5]   */
   val |= (config.comparator_mode&0x1)<<4;          /* bits[4]     */
   val |= (config.comparator_polarity&0x1)<<3;      /* bits[3]     */
   val |= (config.latching_comparator&0x1)<<2;      /* bits[2]     */
   val |= (config.comparator_queue_disable&0x3)<<0; /* bits[1:0]   */

   /* swap the bits... */
   SWAP_BITS(val);

   i2cWriteWordData(i2c_handle,CONF_REG,val);
}

void reset_adc() {

   /* configure ready as a simple trigger */
   set_hithresh(0x0001);
   set_lothresh(0x0000);

   /* write our config */
   write_config();

}

static short read_conversion() {
   short val = i2cReadWordData(i2c_handle,CONV_REG);
   SWAP_BITS(val);
   return val;
}

long next_value(int new_gain) {

   //while (!is_ready()) {
      gpioDelay(1);
   //}

   int ready = is_ready();
   short val = read_conversion();
   //printf("%d %d got val: %d\n",ready,is_ready(),val);
   return (long)val;
}

void close_adc() {

   i2cClose(i2c_handle);

}
