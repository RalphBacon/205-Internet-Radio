The sketch was originally written using the **PlatformIO** IDE but will work just fine in the **Arduino IDE** too.

### For Arduino:

Create (or copy) the folder with the sketch (src) and rename any .cpp files to .ino ones instead.

Place each library file (in lib) into a folder of its own in the usual place (in Arduino-land, there's a **libraries** folder within the sketches folder). Then the compiler will find the libraries when you do the "#include" statement.

Although I've copies a snapshot of the libraries here (because they're the ones I used and therefore know they work) you should always check for newer versions and see if they work too, as there may be fixes and enhancements.

Create your **Data** folder in the same folder as your sketch. This will contain a file called **WiFiSecrets.txt** and you should edit it for your own WiFi's SSID and Password.

You can safely delete the following folders:
* .pio
* .vscode
* include
* lib (**once you have copied them as described above**)
* test
* src (**once you have copied the files to your Arduino's sketch folder**)

You can safely delete the following files:
* gitignore
* no_ota.csv (just use the same ESP32 partition settings as last week)
* platformio.ini
* replace_fs.py

### For PlatformIO
This is the complete project, you know what to do to include it into your workspace. (Note, you may need to edit your workspace folder at the same level as your Project's folder) to something like this:
```
"folders": [
		{
			"name": "ESP32 Better Internet Radio",
			"path": "Projects\\ESP32 Better Internet Radio"
		}
	],
  ```
  It will then appear in PlatformIO's workspace list of projects.
