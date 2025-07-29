# JammaMia
JammaMia open source firmware, to be used with JammaMia PCB REV.1

Compile with Arduino IDE 2.x.

Arduino libraries needed:
- Arduino's Keyboard
- Arduino's Mouse
- Adafruit MCP23017 (https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library) with dependency Adafruit BusIO (https://github.com/adafruit/Adafruit_BusIO)
- DigitalWriteFast (https://github.com/ArminJo/digitalWriteFast)
- ArduinoJoystickLibrary from Matthew Heironimus (https://github.com/MHeironimus/ArduinoJoystickLibrary)

Specific JammaMia libraies (in this github):
- KeyboardNKey for 24-Key rollover (https://github.com/njz3/jammamia/tree/main/Libs/KeyboardNKey)
- MouseN for 2 mices emulation (https://github.com/njz3/jammamia/tree/main/Libs/MouseN)

# Technical information

The board contains 2xMCP23017 IOs expenders on I2C bus for digital inputs (buttons) and outputs (lamps);
The Atmega32u4 IOs are routed to either analog inputs, or PWM enabled outputs. A ULN2003 chip is used for the OUT signals.
The OUT pins are meant to be used to set GND to the pin when it is enabled. Maximum current is 500mA and maximum voltage is +5V, taken from the cabinet power supply.
The OUTPWM pins have also the additionnal property to be PWM pins that can be used to perform dimming or control a solenoid power transistor.

Example wiring for a bulb LAMP:
````
      +5V
       |
      LAMP
       |
      OUT1   (will be pulled to GND when enabled)
````

# Routed IOs

See pinout in PDF: 
![Pinout](https://github.com/njz3/jammamia/blob/main/JammaMia%20-%20Pinout.pdf)

# Serial port commands

The board is viewed as a serial port (COM under windows, or /dev/ttyUSBxx on Linux). When you connect to it, the baudrate is 1000000 baud.
Each command is a text command ended by the '\n' (newline) character.
Some commands are single char only, other longer forms use a '$' escaping character.
When a reply is expected, the reply will start with a 'M' for a standard reply message, a 'S' for the status message, a 'E' for an error message. Each reply line will end with a '\n' character.

## Available commands

o short commands:
- ```v```: printboard version.
- ```l```: list current din (digital in)/ain (analog in) configuration, one per line.
- ```u```: give inputs value.
- ```s```: enable streaming of inputs values.
- ```h```: halt streaming of inputs values.
- ```o```: set digital outputs value 0..F (only 4 bits). Syntax: ```oXX``` with XX being a value between 0..F that enable/disable an output.
- ```p```: set pwm block analog out value. Syntax ```pXYY``` with X being a 4-bit selector and YY being a value between 0..FF.
- ```~```: reset/restart board.

o long escaped commands:
- ```$resetcfg```: reset board configuration to its default (not saving to eprom).
- ```$savecfg```: save board configuration to eprom.
- ```$loadcfg```: load board configuration from eprom.
- ```$get param```: get the value of a parameter, value will be printed as an HEX(adecimal) value like ```FF```. List of parameters given below.
- ```$set param=HEX```: set the value of a parameter, value must be an HEX(adecimal) value like ```FFF```. List of parameters given below.
- ```$setdin DIN TYPE MAP SHIFTEDMAP NAME```: set the configuration of a digital input DIN. See below for more details.
- ```$setain AIN TYPE POS NEG DMIN DMAX NAME```: set the configuration of an analog input AIN. See below for more details.

## List of parameters

- ```delay```: add a loop delay in microseconds to lower the refresh rate and save USB resources.
- ```kblay```: keyboard layout. 0=USA, 1=FR, 2=DE, 3=IT, 4=ES. Default value is 1 (FR).
- ```emode```: emulation modes. 0=no emulation, 1=kerboard only, 2=Joystick only, 3=joystick and keyboard, 4=mouse, 5=mouse and keyboard. Default value is 3.
- ```axes```: number of emulated axes for each gamepad. Default value 2.
- ```btns```: number of emulated boutons for each gamepad. Default value is 0xA (=10)
- ```hats```: number of emulated HAT switch for each gamepad. Default value is 2.
- ```shift```: digital input used for shifted mapping. Default value is 0.

## Configuration of DIN

For digital inputs, din configuration value are in the following order: 
```DIN TYPE MAP SHIFTEDMAP NAME```

Meaning is:
### DIN
Digital input number in HEX format (no 0x prefix needed)

### TYPE
Type value in HEX format (no 0x prefix needed).

Here is the enumerated values:
- 0=Nothing,
- 1=Emulation of a keyboard key (a,z,w,q,e,etc.),
- 2=X/Y/Z analog axes, __NOT SUPPORTED for DIN__,
- 3=HAT 8 directions HAT (see HATDirections),
- 4=Joystick buttons,
- 5=mouse axes X/Y/Wheel from analog or digital,
- 6=mouse button left/right/middle/prev/next.

### MAP
Mapping value in HEX format (no 0x prefix needed)

Value is index of keyscan code (0 being none), joy or mouse axis, joy or mouse button.
For axis and buttons, the 7th MSB (0b10000000) gives the player selection P1-P2, bits 6 to 0 are axis or button index.
For HAT switch, the 7th MSB gives the player selection P1-P2, 5&6th gives the hat switch number, 3 to 0 gives the direction.

### SHIFTEDMAP
Shifted mapping value (0 for none), in HEX format (no 0x prefix needed).

Index of keyscan code when using shifted/alternative map (0 being not used/none).
 
#### NAME
Optionnal name of input (limited to 3 char).

## Configuration of AIN

For analog inputs, ain configuration value are in the following order: 
```AIN TYPE POS NEG DMIN DMAX NAME```

Meaning is:
### AIN
Digital input number in HEX format (no 0x prefix needed)

### TYPE
Type value in HEX format (no 0x prefix needed).

Here is the enumerated values:
- 0=Nothing,
- 1=Emulation of a keyboard key (a,z,w,q,e,etc.), __NOT SUPPORTED for AIN__,
- 2=X/Y/Z analog axes,
- 3=HAT 8 directions HAT (see HATDirections),
- 4=Joystick buttons,
- 5=mouse axes X/Y/Wheel from analog or digital,
- 6=mouse button left/right/middle/prev/next.

### POS
Mapping value when going in positive direction, in HEX format (no 0x prefix needed)

Value is index of keyscan code (0 being none), joy or mouse axis, joy or mouse button.
For axis and buttons, the 7th MSB (0b10000000) gives the player selection P1-P2, bits 6 to 0 are axis or button index.
For HAT switch, the 7th MSB gives the player selection P1-P2, 5&6th gives the hat switch number, 3 to 0 gives the direction.

### NEG
Mapping value when going in negative direction, in HEX format (no 0x prefix needed)

Value is index of keyscan code (0 being none), joy or mouse axis, joy or mouse button.
For axis and buttons, the 7th MSB (0b10000000) gives the player selection P1-P2, bits 6 to 0 are axis or button index.
For HAT switch, the 7th MSB gives the player selection P1-P2, 5&6th gives the hat switch number, 3 to 0 gives the direction.

### DMIN
Dead zone min value if hat or button, usually 60

### DMAX
Dead zone max value if hat or button, usually 80
 
#### NAME
Optionnal name of input (limited to 3 char).

