# tokoLights-led

this is a sketch for an ESP8266, created in the Arduino IDE to let the mcu control three different led-strips. Two single-color strips can be dimmed in a spectrum from 0 to 1023 while a third strip of neopixels can be set to a number of color animations or solid colours.
The sketch allows the mcu to connect to a wifi, subscribe to a mqtt broker and communicate over that channel. It prepares the microcontroller for Over The Air updates and it fits neatly with the commands sent by the tokoLights-daemon in order to illuninate a 3d printer in sync with the states provided by an octoPrint installation controlling that 3d printer.

It relates to these repo and things:
- tokoLights-daemon, a python script processing mqtt messages sent by octoPrint https://github.com/planetar/tokoLights-daemon
- enclosure for a raspberry camera with attached led-ring https://www.thingiverse.com/thing:3984749
- rear column for the Ender 3 to attach the camera enclosure to https://www.thingiverse.com/thing:3978597
