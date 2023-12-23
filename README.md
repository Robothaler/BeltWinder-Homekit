# BeltWinder
GW60 Superrollo belt winder ESP32 program with Matter support
Version: 1.0

Inspired and based on the program from ManuA and mar565:
https://gitlab.com/ManuA/GW60-Superrollo
https://github.com/mar565/BeltWinder

Designed for the board from Papa Romeo (https://forum.fhem.de/index.php/topic,60575.msg723047.html#msg723047)

When, and if, I work off something from the ToDo list decides desire and mood.
Help is of course welcome. Since I am not a computer scientist, but only do this as a hobby, I like to learn in this way.

Librarys:
Please add the following library:
https://github.com/jakubdybczak/esp32-arduino-matter.git

# Features:

**Integration via QR code or via code input:**.   
    * QR-Code: https://project-chip.github.io/connectedhomeip/qrcode.html?data=MT%3AY.K9042C00KA0648G00  
    * The calibration run must be carried out in the app after the first installation, after the end stops of the GW60 have been set. To do this, the light switch in the app (I have named this "Calibration") must be pressed.
* After reboot:
    * last known position is transmitted. Position is unconfirmed
    * If a percentage value unequal 0/100 is to be approached first, a positioning run takes place [shortest distance between presumed position and desired position, with stop at a stop]. Position is confirmed until next reboot


# To-Do's
* I still don't like the solution with the light switch as the activation switch for the calibration run. I want to try further improvements here. Mode Select or Mode Base may be able to help here.
* Integration of Matter's OTA functionality to easily update the installed belt winders.

ATTENTION:
I have put this version into operation at my place, it works so far. Let's see what bugs come up, if necessary I'll add bug fixes and feature improvements.
Use the code at your own risk.
