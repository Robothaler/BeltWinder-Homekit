# BeltWinder
GW60 Superrollo belt winder ESP32 program with Matter support
Version: 0.1

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
    * Calibration drive must be started: once necessary.
    * Position Topic is created and activated
* After reboot:
    * last known position is transmitted. Position is unconfirmed
    * If a percentage value unequal 0/100 is to be approached first, a positioning run takes place [shortest distance between presumed position and desired position, with stop at a stop]. Position is confirmed until next reboot


# To-Do's
* beautify readme...
* tests as soon as Apple has released the update to 16.2

ATTENTION:
This is an untested version, the communication to the Matter SDK is not yet defined.

Translated with www.DeepL.com/Translator (free version)
