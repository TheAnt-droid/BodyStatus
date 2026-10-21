# Body Status Analyser
## Requirements
### Hardware
CY8KIT-5LP Set, Environment 3 click, Oximeter 5 click
### Goal
Use the given hardware to create a Body Status Analyser
Function Description
When a finger is placed on the Oximeter 5 and the button SW1 is
pressed, the evaluation of pulse and SpO2 starts. At the same time,
the ambient atmosphere is analysed by Environment 3. The results
of the analysis are indicated via the LED. The red LED pulses in sync
with the heartbeat, the luminosity of the blue LED depends on the
oxygen saturation. The green LED encodes the data from the
Environment 3 regarding temperature, pressure, humidity, and gas.
If all values are in an acceptable range for an indoor environment,
the green LED is fully lit, if one or more values are off, the green LED
blinks.
### Hardware Configuration and Communication
For the ENVIRONMENT 3 the SPI interface shall be used so it must be
in slot 1 and the OXIMETER 5 that uses the I2C interface is seated in
slot 2. The switch S1 must be set to BTN, and the switch S2 SPI
Bridge must be set to ON.
### Further Notes
What is considered an "acceptable range " is up to the developer.
Whether the data is read directly from the Environment 3, the Bosch
Sensor API or the "API and BSEC 2.x library" is used, is also up to the
developer. 
