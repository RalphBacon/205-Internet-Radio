### Arduino Version
<img src="images/v1.19_Arduino_IDE_Version.jpg" width="25%">
**v1.19 Stable version for Arduino IDE**
  * On screen volume & screen brightness indicator, stored in EEPROM
  * General tidy up of the code
  * Station lister tidied (press the antenne in the title bar to invoke)
  
**v1.x General proof of concept version**

If you already have this project, you an either copy it as an archive elsewhere (so the compiler cannot find it) or update it.

* Create a new folder in your Arduino sketches folder called "ESP32-WROVER_Web_Radio"

* Copy the unzipped project into that folder.

* Follow the instructions in the "libraries" folder in this project.

* Upload the "data" folder using the "ESP32 Sketch Data Upload" LittleFS/SPIFFS upload utility

* Update the WiFiSecrets.txt file with YOUR SSID and PASSWORD (Before you ask, "Benny" is not the password to my SSID)

* Edit the "main.h" file and reduce the circular buffer from 150000 to 10000 (for now - I have yet to find a way to hack the ESP32 circular buffer for the Arduino environment)

* Open, compile and upload the "ESP32-WROVER_Web_Radio.ino" sketch to your ESP32 device  
(Note that several files [tabs] will open in the Arduino IDE)

* Look at the serial monitor output to find out whether anything went wrong

You should (on boot up) hear a welcome message and then a station will tune in automatically.

If the screen goes dark after a few seconds, press the "+" button repeatedly on next boot and the screen will reappear. I don't know why it goes dark for some users, initially, as the default value is full brightness.

To change stations click the Antenna icon in the title bar.
