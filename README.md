
# Motor Stand

Work on the motor stand for testing the rocket motor.   
Written for 2025/2026 year.

## Usage

### Prerequisites

Sudo access is needed to run this program. Additionally, access to
the GPIO pins is handled through the pigpio library. Documentation
can be found at the following url:

```
https://abyz.me.uk/rpi/pigpio/index.html
```

This library can be installed via the following series of commands:

```
mkdir -p pigpio_install
cd pigpio_install
wget https://github.com/joan2937/pigpio/archive/master.zip
unzip master.zip
cd pigpio-master
make
sudo make install
cd ../..
```

### Building

This can be done by running the build script. This will compile
all programs from source code.

```
./build.sh
```

Alternatively, you can use CMake which works on Linux and Windows (MSYS2/MinGW):

```
cmake -S . -B build
cmake --build build --config Release
```

Notes:
- On Raspberry Pi / Linux the pigpio library and librt are used by the server; install pigpio before building (see above).
- On Windows/MSYS2 the pigpio and librt libraries are usually not available; the build script will still compile the non-hardware parts. For the GUI build install GTK3 (via MSYS2 pacman) and use CMake.

### Usage

This code is written as a client-server system. The server will
broadcast all data it collects to the address it is provided by
the user. The port is fixed as 8777.   

#### Server
   
The server can be ran using the following command:
```
sudo ./build/load_cell_serv.e [ -c | -k] <known_weight/multiplier> <address>
```
The option *-c* indicates that the second value passed is a known weight
that will be used for calibration. This will cause the server to first
prompt for the user to place the known weight on the load cell and press
enter. The server will then calculate the multiplier and print it to
the shell. Following this, it will start collecting data.    
     
The option *-k* should be called after a calibration has occured. It
indicates that the second value passed is a multiplier from a previous
calibration. This value will be used as the multiplier and calibration
will be skipped.     
    
The dest address parameter expects the IPv4 address in dotted-decimal
notation (i.e. XXX.XXX.XXX.XXX) that the server is to send packets to.
This may be a broadcast address, as the server will be configured with
the broadcast option.

#### Logger

The logger is a client program that is provided. It writes the
collected output data to a CSV file of a user-specified name.
The CSV is written in append mode, so multiple runs may be provided
the same file name. Simply search for the header in the file and 
delete any multiple occurances.    
    
This program can be run using the following command:
```
./build/load_cell_logger.e <file name>
```
where file name is the name of the file. Passing "-" as the file
name will result in output being printed to stdout.

#### Client Interface

Additional client programs can be written by collecting UDP packets
or by using the function given in the src/load_cell_cli.h header file.
This function will run a given callback every time a packet is recieved,
and takes away some of the socket-based setup. For example, to spawn
three threads that print the seconds, nanoseconds, and microseconds of
each packet, only the following code must be written:

```C

void handler_sec(output_data * o, void * uarg) {
   printf("%ld\n",o->time_collect_sec);
}
void handler_nsec(output_data * o, void * uarg) {
   printf("%ld\n",o->time_collect_nsec);
}
void handler_usec(output_data * o, void * uarg) {
   printf("%ld\n",o->time_collect);
}

int main(void) {

   run_load_cell_cli(handler_sec,NULL,LC_CLI_DETACH);
   run_load_cell_cli(handler_nsec,NULL,LC_CLI_DETACH);
   run_load_cell_cli(handler_usec,NULL,LC_CLI_DETACH);

   while (1)
      sleep(1);

   return 0;
}

```

See the src/load_cell_logger file for an additional example.

## Hardware

### ADS1115 (current)

This is intended to be ran on a raspberry pi connected to the
ADS1115 ADC. The software expects gpio 2 and 3 to be connected
to the SDA and SCL pins on the ADS1115 respectively. The gpio
17 is expected to be connected to the ALERT/RDY pin on the ADS1115.
It operates with the default I2C addressing mode, so ADDR should
be connected to GND.

```

 GPIO  2 ---> SDA
 GPIO  3 ---> SCL
 GPIO 17 ---> ALERT/RDY

 OR
 
 PIN  3 ---> SDA
 PIN  5 ---> SCL
 PIN 11 ---> ALERT/RDY

```

I would run this at 3.3 V power, as it accepts 2V-5V:

```

 PIN 1 ---> VIN
 PIN 6 ---> GND

```

// TODO -> ADC connection

### HX711 (outdated)

As of today ``01/13/2026`` the hx711 is no longer being used in
this program. If you would like to run with it again you will
have to replace the ads1115 header file in the load_cell_serv.c
file with the hx711 file and change the compilation instruction
to include the hx711.c instead of the ads1115.c.

This is intended to be ran on a raspberry pi connected to the
HX711 ADC. The software expects gpio 23 to be connected to
the DT pin on the ADC and gpio 24 to be connected to the
SCK pin on the ADC.

```

 GPIO 23 ---> DT
 GPIO 24 ---> SCK

 OR

 PIN 16 ----> DT
 PIN 18 ----> SCK

```
The ADC is expected to be connected to a load cell on the A+ and A- 
connections, as the software does not collect data from the B+ or B-
connections. If you would like to do so, Change all instances of
*GAIN_128* in src/load_cell_server.c to *GAIN_32*.


## Send Complaints

qeftser

