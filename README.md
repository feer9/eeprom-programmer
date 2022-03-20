
# EEPROM Programmer

Utility to read and write to I2C EEPROM memory devices.


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
- 24LC64 (not yet)
- X24645
- 24LC256 (not yet)

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

## PC CLI

PC CLI made with Qt's QSerialPort.   
Run `eeprom-programmer -h` to get command line options

## USB CDC
USB CDC Class implemented thanks to philrawlings repo:   
https://github.com/philrawlings/bluepill-usb-cdc-test

