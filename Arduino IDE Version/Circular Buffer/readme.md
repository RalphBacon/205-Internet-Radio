1. Identify the location of your ESP32 Core library. For me it's:
C:\Users\Ralph\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\cores\esp32

2. Find the cbuf.cpp file and make these amendments:

  * Find the **cbuf::resize** function
  * Find the lines:  
    char \*newbuf = new char[newSize];  
    char \*oldbuf = \_buf;  
  * Replace them with:  
    // RSB Use PSRAM here if required  
	  char \*newbuf;  
	  if (BOARD_HAS_PSRAM)  
	  {  
		  newbuf = (char \*)ps_malloc(newSize);  
	  }  
	  else  
	  {  
		  newbuf = new char[newSize];  
	  }  
	  char \*oldbuf = \_buf;  

3. If it makes it easier, grab the modified cbuf.cpp file above and replace your copy (best make a backup first, hey?).

4. Now when you compile, it will allocate the circular buffer in PSRAM (shown on startup in the Serial Monitor).

<img src="images/UsingCirBuffer.JPG">

5. Once you are sure it is using the PSRAM (not SRAM) increase the cbuf value in file main.h to 150000 (150K) and you have a buffer that gives about 10 seconds, more than enough.

<img src="images/main.h_edits.JPG>