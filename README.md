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
- KeyboarNKey for 24-Key rollover (https://github.com/njz3/jammamia/tree/main/Libs/KeyboardNKey)
- MouseN for 2 mouses emulation (https://github.com/njz3/jammamia/tree/main/Libs/MouseN)

# Technical information

The board contains 2xMCP23017 IOs expenders on I2C bus for digital inputs (buttons) and outputs (lamps);
The Atmega32u4 IOs are routed to either analog inputs, or PWM enabled outputs (dimming).
The OUT pins are meant to be used to set GND to the pin when it is enabled. Maximum current is 500mA and maximum voltage is +5V.
The OUTPWM pins have also the additionnal property to be PWM pins that can be used to perform dimming.

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
