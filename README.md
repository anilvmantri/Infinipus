## Infinipus LED/Encoders using RS485 modules/Adafruit I2C Stemma QT + Rotary Encoders

Contains schematics & code for basic communication between two microcontrollers (here, Nano Everys) using UART/RS485 modules. In this case, the master arduino polls the slave arduino - which has a rotary encoder attached. The slave sends back the current rotary encoder value which is then used to update the color of the NeoPixel strand(s) attached to the master.

Basic schematic is below - Arduinos are connected to RS485 modules which are connected to RJ45 breakout boards - which themselves are connected via an Ethernet cable.

![alt text](https://github.com/anilvmantri/Infinipus/blob/main/documents/infinipus_schematic.png?raw=true)
