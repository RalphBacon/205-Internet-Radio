## ESP32 Construction Details (with Photos)

December 2020

#### STEP 1

![Step 1.jpg](I:\Documents\GitHub\ESP32%20Web%20Radio%20Construction\images\Step%201.jpg)

Sort out the correct length pin header sockets and pins. They are always 2.5mm (0.1") spacing, same as a breadboard.

You need a 14-way socket for the screen PCB (plus a 4 pin socket that is not electrically connected).

**NOTE: The screen socket (14-way) goes on the FRONT of the board (without the writing) and the 20-way (2x10) goes on the BACK of the board (with the writing).**

The ESP32 TTGO T8 V1.7.1 requires two 15-pin header sockets to plug into. Mount the sockets onto the PCB and the 15-pin headers (that are usually included with the module) to the ESP32 module.

![Step 1A.jpg](I:\Documents\GitHub\ESP32%20Web%20Radio%20Construction\images\Step%201A.jpg)

You also need two strips of 10 header pins for the TFT PCB. Once again, **note that the pins are mounted from the rear**. Note sure? Look at some of the other photos before soldering anything!

### STEP 2

<img title="" src="file:///I:/Documents/GitHub/ESP32%20Web%20Radio%20Construction/images/Step%202.jpg" alt="Step 2.jpg" data-align="center" width="469">

Solder each of the header sockets / pin headers by **first tacking each end** to ensure they are totally vertical. When you are satisfied that they are good, continue soldering all the rest of the pins for that strip. 

I use a **hot** soldering iron (400ºC) for this stage with some extra flux. Be quick or the plastic around the pin headers will melt causing the pins to lean over.

Clean off all remaining flux by using isopropyl alcohol and a horsehair or nylon brush.

Do this for both PCBs. This is the most time consuming aspect of the project but it is essential that the pins and sockets are 100% vertical or you won't be able to get the IDC cables on, nor the screen to fit.

### STEP 3

![Step 3.jpg](I:\Documents\GitHub\ESP32%20Web%20Radio%20Construction\images\Step%203.jpg)

Your header sockets / pins are fitted and the flux removed. 

Fit the single 10K resistor between the header sockets for the TTGO. You can use either a surface mount or through-hole component here **but not both**.

Fit the *optional *SMD 100nF capacitors by the 5v pin header before soldering the two-pin 5v headers (I used right angle pin headers here but vertical work too). finally, you can fit a 100nF SMD capacitor on the screen PCB. All belt and braces to remove noise.

Don't fit the large, electrolytic capacitor by the 5v power pins yet.

![IMG_20201226_133340.jpg](I:\Documents\GitHub\ESP32%20Web%20Radio%20Construction\images\IMG_20201226_133340.jpg)

Mount the VS1053 on standoffs, high enough so that the jack socket clears the pin headers when the IDC cables are fitted.

Fit the power cable (mine goes to a 2.1mm/5.5mm socket), **getting the polarity correct**. Support the cable by using a cable tie but don't do what I did in the photo above; the cable tie should go around the full (black) cable not just the two wires inside it (yes, I've corrected it now).

Now (optionally) fit the electrolytic capacitor, ensuring it fits snugly next to the VS1053, and that it is rated at 10 volts or above. I used a 470μF and it fitted easily but YMMV (Your Mileage May Vary). Get the polarity right or it will explode!

### STEP 4

![Step 4.jpg](I:\Documents\GitHub\ESP32%20Web%20Radio%20Construction\images\Step%204.jpg)

Nearly done. 

Secure the VS1053 as shown, and connect it via the 10-way IDC ribbon cable to the motherboard, getting the polarity of the cable correct. The red stripe goes from the 5v pin to the motherboard's 5v pin.

Connect the screen's 20-way cable, with the red stripe going from 5v on the motherboard to the 5v on the screen PCB.

Connect one end of the 3.5mm jack to the VS1053 and the other to your amplifier's or computer's LINE-IN socket.

If you want to implement the ground loop transformers, I have some PCBs on order to do this so watch for an update here. Or just use the AUKEY ground loop device I showed from Amazon (https://amzn.to/2HyMqn1)

### Step 5

Power up.

Assuming you have already uploaded the sketch and SPIFFS partition with all the relevant files (intro.mp3, WiFiSecrets.txt etc) with the partitions split into 2Mb/2Mb/No OTA you should be good to go.

![IMG_20201226_133516.jpg](I:\Documents\GitHub\ESP32%20Web%20Radio%20Construction\images\IMG_20201226_133516.jpg)

The screen should burst into life and you should hear "ESP32 Web Radio". This start up message proves that the code is loaded and the VS1053 is connected correctly. It's pretty much the first thing that runs, intentionally so.

Check the Serial Monitor output from the ESP32's USB port. You should see various messages about connecting to your Wi-Fi, as well as what SRAM and PSRAM you have available.

You should hear the first radio station listed in the code. (Don't worry, radio stations will be added more slickly in the future, keep tuned).

If not:

- check that you have the IDC cables the right way round (don't guess!)

- check that you have power coming into the board (use a multimeter), on my version there are no power LEDs

- disconnect the screen, the sketch will run without it, press reset button

- check the serial monitor output, that will give the biggest clue(s)

### And Finally...

I am continuing to (slowly) update this project. However, I am using PlatformIO. If you are using that too then you can just take the entire project and compile it on your own PC.

If you are using the Arduino IDE, then I have to copy the code to my Arduino environment which happens every now and again.

If you're serious about developing sketches larger than the BLINK sketch please consider using the PlatformIO IDE (using Visual Studio Code). It gives you IntelliSense (hints to parameters and functions), colour coded keywords and doesn't let you make silly errors that the Arduino IDE makes you discover only when you compile your code.
