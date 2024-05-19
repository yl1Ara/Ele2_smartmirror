This file contains the data sent to the esp32s2. Most library data is not included here as they are downloaded from Arduino IDE libraries!!! Only if they needed changes the changes are included here.

- The main ESP32 code is the espcode.ino made using Arduino IDE
- The weathercode pngs are also here (sunny, cloudy etc.). These are the byte arrays that our maind code uses to push png images on the OLED.
- in the pitches folder is our pitches that we use. These are bit different from the normal library pitches so they are included
- The TFT_eSPI librarys user settings need to be changed to match our OLED system and the pins we use. The corrected file is included TFT_eSPI/User_Setup.h 
that can then be changed in the library file if you want to use same schematic configuration for pin layout


Made by, 
( ͡° ͜ʖ ͡°) tech. Chief of 3d modelling, schematics and MCU coding Arttu Yli-Kujala