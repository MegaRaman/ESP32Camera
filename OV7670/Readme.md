# Facial recognition with OV7670 and face_recognition Python module
This repository is a fork of bitluni's project with the added support of facial recognition and slightly different 
[LCD module](https://www.waveshare.com/wiki/2.4inch_LCD_Module)

## Usage
Start a server with ./camera_server.py and connect camera with LCD module, default pinout is following:
LCD_CS_PIN -> 16  
LCD_DC_PIN -> 15  
LCD_BL_PIN -> 19  
LCD_RST_PIN is unconnected  

CAM_VSYNC -> 32  
CAM_SIOD -> 21  
CAM_SIOC -> 22  
CAM_RRST -> 17  
CAM_WRST -> 12  
CAM_RCK -> 4  
CAM_WR -> 5  
CAM_OE -> GND  
CAM_PWDN not nonnected  
CAM_HREF not connected
CAM_STR not connected
CAM_RST -> 3.3V 

CAM_D0 -> 13  
CAM_D1 -> 33  
CAM_D2 -> 14  
CAM_D3 -> 27  
CAM_D4 -> 26  
CAM_D5 -> 25  
CAM_D6 -> 35  
CAM_D7 -> 34  

Pinout can be changed by editing `DEV_Config.h` for LCD pinout and `OV7670.ino` for camera pinout
Allowed people faces should be put in `allowed_people/` directory

## Dependencies
- face_recognition
- Flask

