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

The following commands are available:

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
- ```$rsetcfg```: reset board configuration to its default.
- ```$savecfg```: save board configuration to eprom.
- ```$get param```: get the value of a parameter, value will be printed as an HEX(adecimal) value like ```FF```.
- ```$set param=HEX```: set the value of a parameter, value must be an HEX(adecimal) value like ```FFF```.
- ```$setdin DIN TYPE MAP SHIFTEDMAP NAME```: set the configuration of a digital input DIN. See below for more details.
- ```$setain```: set the configuration of an analog input AIN. See below for more details.

