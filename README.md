# 1PPM-timebase-for-impulse-clock-movement
ESP-01 NTP 1PPM (pulse-per-minute) timebase for impulse clock movement
This sketch outputs a .25 second pulse every minute synced to NTP (internet) time.
You will have to figure out how best to connect this to your clock movement. Mine happens to be a 10 ohm coil,
and powers well from the same 3v power supply as the ESP-01.
I'm feeding the output pin to the base of an NPN transistor through a 1k ohm resistor.
Emitter to ground and collector to one side of the coil which has 3v on the other. Don't forget a 1N400x acoss the coil
for flyback control

There is a very simple web interface on port 80 of the ESP which wll allow you to advance or delay a number of seconds
which allows adjusting for DST/STD time.

This is relatively power hungry, so not really suited for battery usage.
