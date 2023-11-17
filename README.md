# USdigital A2 encoder interface to Modbus-RTU RS-485 interface
![image](https://github.com/EA3HMJ-Tracking-Software-Suit/A2-Interface/assets/2368602/8dd7b06c-fc4e-4970-8531-5e5ef3688cf7)

Interface based on the ESP32 S2 Mini microcontroller and the ESP32 S2 Mini library [eModbus](https://github.com/eModbus/eModbus).

It behaves as a modbus-RTU client with addresses 1 for azimuth and 2 for elevation at 115200 bauds and responding to function 3 and 6.

Register 0 returns a 32-bit number (two registers) which is the encoder position.
