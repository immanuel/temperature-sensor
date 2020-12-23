# Low-Power Wireless Temperature Sensor 

The embedded code is compiled to run on a Cypress PSoC 4 system-on-chip with bluetooth-low-energy (BLE) sub-system and ARM Cortex CPU. For temperature sensing, a simple voltage divider circuit is used to measure the resistance (relative to a standard resistance) of a thermistor which varies with ambient temperature.

By invoking the SoC's low-power mode, the circuit is powered by a coin cell, enough to power it for months. 

The module is programmed to repeatedly advertise the ambient temperature over BLE, allowing a central server to pick up the readings. 

The BLE module is the CY8CKIT-143A PSoC® 4-BLE 256KB Module with Bluetooth 4.2 Radio manufactured by Cypress Semiconductors. The module can be programmed with the development board that comes with the Pioneer Kit (CY8CKIT-042-BLE-A).

