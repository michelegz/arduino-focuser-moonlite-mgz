# 🌙 Arduino Focuser Moonlite Compatible

[![Work in Progress](https://img.shields.io/badge/status-work%20in%20progress-orange.svg)](https://github.com/your-repo/arduino-focuser-moolite-mgz)


A Moonlite-compatible stepper controller for telescopes, based on original work by Orlando Andico, with several enhancements and support for different motor drivers.

## 🚀 Features

- **Moonlite Compatibility**: Works with both Moonlite and Ardufocus ASCOM drivers
- **Multi Motor Driver Support**: 
  - Adafruit Motor Shield V1 (L293D)
  - Generic H-Bridge stepper driver (L298N, etc.)
- **Half-Step Support**: Half-step mode implementation
- **Temperature Monitoring**: DHT11 temperature sensor integration
- **Position Saving**: Last position saved to EEPROM
- **Speed Control**: Speed adjustment from Moonlite GUI
- **Flexible Configuration**: Separate configuration files for different Arduino boards

# ⚠️ Important notes

*This applies only to Moonlite ASCOM driver and NOT to Ardufocus driver*

Requires a 10uf - 100uf capacitor between RESET and GND on the motor shield; this prevents the Arduino from resetting on connect (via DTR going low).  Without the capacitor, this sketch works with the stand-alone Moonlite control program (non-ASCOM) but the ASCOM driver does not detect it.
Adding the capacitor allows the Arduino to respond quickly enough to the ASCOM driver probe

# 📄 License
Based on original work by Orlando Andico, released under BSD 2-Clause "Simplified" License.