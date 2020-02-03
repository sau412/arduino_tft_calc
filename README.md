# Arduino TFT calculator with sensor screen

Hardware:
* Arduino Uno (with ATMega328P)
* TFT screen with sensor (I use one with controller r61505 ili9325)

Required libraries:
* Adafriut GFX library https://github.com/adafruit/Adafruit-GFX-Library
* Adafruit TFTLCD library https://github.com/adafruit/TFTLCD-Library
* Adafruit TouchScreen library https://github.com/adafruit/Adafruit_TouchScreen

Not completed yet:
* Screen calibration on start
* Calculation of a^b for all ranges

Completed:
* Expression parsing
* 20-digits (decimal digits) precision numbers
* Triginometric functions
* Exponential and logarithmic functions
* Constants
* Seven memory registers (A-F, M)

