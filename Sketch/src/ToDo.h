/*
	Somewhere for me to put all the things I need to FIX or DO, including some
	experimental work.

	TODO:

	1 - Buffer the input stream so that the VS1053 doesn't depend on the latest
		32-byte chunk that's just been read from the http: datastream. This may 
		require putting	the play function into a separate task. 32 bytes gives 
		1 mS of playing time at 128Kbps so an SRAM buffer of 10000 bytes gives
		about 310mS of buffering time. PlatformIO says we are only using about
		53K of SRAM and we have 327Kb to play with. FIXME DONE

	2 -	Do the Next and Prev buttons properly, possibly via interrupts but we must
		ensure not to impact the data stream buffering (too much) FIXME

	3 -	Display the stream bits per second (bitrate) on screen just FYI, as it can
		determine the overall, perceived quality (eg muddy sound vs clear sound),
		or explain why the stream is stuttering (not receiving data quickly enough)
		ENHANCEMENT DONE
	
	4 -	Split the Artist & Track string and display on separate lines on screen,
		to mitigate the chance of the string wrapping onto the next line ENHANCEMENT

	5 - When display the Artist / Track, calculate the width and only split on
		word boundary so we don't split a word on screen, looks ridiculous ENHANCEMENT
	
	6 - Ensure all debugging messages are wrapped in an #if...#else so they do not
		impact performance of final project FIXME
	
	7 - Ensure critical errors are displayed on screen (eg WiFi problem, station not
		connected to after X attempts, too many small read errors [BBC Radio 4 worst
		culprit]) FIXME

	8 - Store the list of stations in LITTLEFS partition and design web interface to 
		list / edit / play them. ENHANCEMENT

	9 - Sometimes a radio station gets out of sync (rare) but we cannot recover after
		that. Detect invalid ICY data and reconnect to the station.
	
	10 - Mute sound from any station until we have played at least XXX mS to avoid the
		stuttering that always seems to accompany a station change. Check out #1 to see
		whether buffering clears this anomaly FIXME DONE

	11 - Investigate feasibility of connecting BT to phone for streaming audio (eg music
		or podcasts) ENHANCEMENT

	12 - On-Screen "LED" indicators for "Tuned", bad data and buffer level.

	13 - Clear screen track title etc on change of station FIXME

	14 - If metadata contains rubbish characters, take this as indicator that we have not
		synchronised correctly and reconnect to station BUG

	

	

	
	




















*/
