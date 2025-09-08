### Initial setup to program D1 Mini
Board manager -> download esp8266 board (tested with v3.1.2)
When ready to upload select com port and change board to "LOLIN(WEMOS) D1 R2 & mini"
Reference: https://www.airgradient.com/blog/basic-setup-skills-and-equipment-needed-to-build-our-airgradient-diy-sensor/

### D1 Mini Pinouts:
https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/05/ESP8266-WeMos-D1-Mini-pinout-gpio-pin.png?quality=100&strip=all&ssl=1

### Recommendation: 
Before soldering, load 'PT_Test_Tilt_Sensor.ino' onto D1 Mini to verify board operation. Periodically plug board in and monitor serial port to verify soldering hasn't damaged board.

<img width="1396" height="591" alt="Wiring" src="https://github.com/user-attachments/assets/f104e410-4c6e-46f5-b8db-0c12c5f7ba3d" />

### Wiring
D1 Mini\
Solder the tilt sensor directly to board on D4 and D5 (GPIO 12 and 4). The sensor should be soldered to the opposite side of the wifi module and antenna.\
Test with tilt sensor code.\
Solder the following with dupont end wires on the same side as the tilt sensor
* 5V (+5V)
* G (Ground)
* A0 (Analog in)
* D1 (GPIO 6)

NeoPixel\
14 neo-pixels per strip. If different modify puck code.
Use plastic clamps to secure strip to wires. Use around 6" of appropriate gauge/color wire.

Buck converter\
Solder leads to board for all wires except Enable\
Wire battery compartment\
Before connecting to D1 MINI, Hook up battery to Buck converter (recommend aligator clamps). \
Use multimeter to measure voltage. Adjust with on-board pot to +5V.

Connect buck converter to positive of battery\
Connect buck converter output to positive on NeoPixel and +5V on D1 Mini

Knock sensor\
_Seperate from D1 Mini_\
1 Mohm resistor, bend leads to 90 deg at ~1 cm from resistor. \
Strip back wires on piezo element. Solder red and black wires to either side of 1 Mohm resistor. \
Solder ground and signal leads. \
Trim excess resistor wire. \
Use piece of tape wrap around resistor and all solder points, folding tape back on itself to cover all bare wires. Recommend gaffers tape. \
Solder signal wire to A0 on D1 Mini

There should be 5 ground connection: 
* battery
* *knock sensor
* NeoPixel
* Buck converter
* D1 Mini

Start grouping wires into pairs where possible, twisitng and soldering pairs together. \
Once down to three leads, connect all 3 with a Wago. 
