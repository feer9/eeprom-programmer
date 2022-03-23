
# EEPROM Programmer

Utility to read and write I2C EEPROM memory devices.

## Microcontroller

Using a STM32 Black Pill (stm32f103c8)  
and the ST HAL library.  

### Microcontroller connections

Only the I2C2 bus is used

STM32F103  | Function
---------- | -------
PB10       |   SCL  
PB11       |   SDA  

**Important note:**  
Aditionally, a 10k pullup resistor to Vcc is required in both SDA and SCL.

## Memory
Supported I2C memories are:

- 24LC16
- 24LC64
- X24645

Next one in the list:
- 24LC256

### Memory electrical connections

EEPROM (DIP-8) |  Connected to
-------------- | --------------
1 | GND
2 | GND
3 | VCC
4 | GND
5 | SDA
6 | SCL
7 | GND
8 | VCC

## Libraries used

### ST Microelectronics HAL
Using the ST HAL library for I2C, USB, USB CDC, Clocks and GPIO

### Serial port
USB CDC Class implemented thanks to philrawlings modified version of the ST CDC example  
https://github.com/philrawlings/bluepill-usb-cdc-test

### Command Line Interface
The PC side CLI is made with Qt using QSerialPort library among others.  
Run `eeprom-programmer -h` to get command line options

### Special thanks
'sijk' for his implementation on Unix signals in Qt  
https://github.com/sijk/qt-unix-signals
