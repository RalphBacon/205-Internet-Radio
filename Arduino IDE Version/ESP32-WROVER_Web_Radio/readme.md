### Arduino Version

If you already have this project, you an either copy it as an archive elsewhere (so the compiler cannot find it) or update it.

Create a new folder in your Arduino sketches folder called "ESP32-WROVER_Web_Radio"

Copy the unzipped project into that folder.

Follow the instructions in the "libraries" folder.

Upload the "data" folder using the "ESP32 Sketch Data Upload" LittleFS/SPIFFS upload utility

Edit the "main.h" file and reduce the circular buffer from 150000 to 10000 (for now)

Open, compile and upload the "ESP32-WROVER_Web_Radio.ino" sketch to your ESP32 device  
(Note that several files [tabs] will open in the Arduino IDE)

Look at the serial monitor output to find out whether anything went wrong

You should (on boot up) hear a welcome message and then a station will tune in automatically.

If the screen goes dark after a few seconds, press the "+" button repeatedly on next boot and the screen will reappear.

To change stations click the Antenna icon in the title bar.