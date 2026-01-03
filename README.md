# ATC_SR08_Ring Custom firmware demo for the SR08 Smart Ring

Affiliate Link to the Ring:

https://s.click.aliexpress.com/e/_op8fMC1

This repo is made together with this explanation video:(click on it)

[![YoutubeVideo](https://img.youtube.com/vi/xOw-6uMfOjc/0.jpg)](https://www.youtube.com/watch?v=xOw-6uMfOjc)


# Getting Started
Since manual firmware can brick the device, I would highly recommend [the Codeless SDK](https://lpccs-docs.renesas.com/UM-140-DA145x-CodeLess/introduction.html) for those who want to get started.

Here is how:
1. Download the AT command set from here: https://www.renesas.com/en/software-tool/smartbond-codeless-commands?srsltid=AfmBOop--fiuu56b0Nhaiu4AnOmSKZ4o7wKGEFxjuRlS4snoNJ8H11cC
2. Patch it using this version of `make_bin_ota.py`: https://gist.github.com/bsideup/a61bb25cb00494d23af00927908d2cbc
3. Flash it as any other firmware. Don't worry, it supports SUOTA.
4. Use SmartConsole (Android/iOS) app to perform AT commands to play with your ring.

Here is how you can execute "WHO_AM_I" I2C command on the accelerometer:
```at
# Configure I2C using pin 2.3 and 2.4, as per prox_reporter/src/config/user_periph_setup.h
AT+IOCFG=23,7
AT+IOCFG=24,8

AT+I2CSCAN
# Should give you 0x18:0xFE and 0x44:0x27

# Send "WHO_AM_I", as per https://www.lcsc.com/datasheet/lcsc_datasheet_2312221743_Hangzhou-Silan-Microelectronics-SC7A20HTR_C19274408.pdf + ChatGPT
AT+i2CREAD=0x18,0x0F

# Should give 0x11
```

Or turn on the "steps" led:
```
AT+IOCFG=02,4
AT+IO=02,1
```


# Advanced usage (custom firmware)

Use this Android/iOS tool to upload the firmware to the Smart BLE Ring:

https://play.google.com/store/apps/details?id=com.dialog.suota&hl=en
https://apps.apple.com/us/app/renesas-suota/id953141262

Settings as in the OTA_Settings_for_apk.jpg image



The firmware can be compiled with the "SmartSnippets Studio V2.0.20"

The Firmware folder is here: "Firmware_src\projects\target_apps\ble_examples\prox_reporter\Eclipse"

After compiling a custom tool is used to create an OTA Enabled firmware file called "output.bin"


