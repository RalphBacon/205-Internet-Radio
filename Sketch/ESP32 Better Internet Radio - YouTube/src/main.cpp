#include <Arduino.h>
#include "main.h"
#include "tftHelpers.h"
#include "wifiHelpers.h"
#include "taskPlayMusicHelper.h"
#include "stationSelectHelper.h"
#include "settingsHelper.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "urlHelper.h"

// Level is set (see platformio.ini to set this)
/*
	LEVEL_NONE       (0) // Stumm. Nada. On your own.
	LEVEL_ERROR      (1) // Usually fatal
	LEVEL_WARN       (2) // Only when things go wrong
	LEVEL_INFO       (3) // Useful just to see it working
	LEVEL_DEBUG      (4) // Debugging programming
	LEVEL_VERBOSE    (5) // Every message
*/

#define ONESECOND 1000
#define FIVESECONDS 5000
#define TENSECONDS 10000
#define THIRTYSECONDS 30000

// ==================================================================================
// setup	setup	setup	setup	setup	setup	setup	setup	setup
// ==================================================================================
void setup()
{
	// disable brownout detector
	// WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

	/*
		To re-enable the brown out detector, do this:

		// enable brownout detector
		WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); 
	*/

	// Debug monitor
	Serial.begin(115200);

	// Version number (long format, incl. date eg "v1.210123_5 - 2021-01-23 13:38:26.167703")
	log_n("Version %s", VERSION);

	// Initialise EEPROM helper
	preferences.begin("WebRadio", false);

// Are we using PSRAM?
#ifdef BOARD_HAS_PSRAM
	log_d("This board has PSRAM installed.");
#else
	log_d("This board has no PSRAM installed.");
#endif

	tft.println("Setup creating ring buffer.");
	circBuffer.resize(CIRCULARBUFFERSIZE);
	log_d("Total heap: %d", ESP.getHeapSize());
	log_d("Free heap: %d", ESP.getFreeHeap());
	log_d("Total PSRAM: %d", ESP.getPsramSize());
	log_d("Free PSRAM: %d", ESP.getFreePsram());
	log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());

	// Station change pin
	pinMode(stnChangePin, INPUT_PULLUP);
	pinMode(tftTouchedPin, INPUT_PULLUP);
	pinMode(ICYDATAPIN, INPUT); // Cannot be pullup, input only pin

	// initialize SPI bus;
	log_d("Starting SPI");
	SPI.begin();

	// Initialise LITTLEFS system
	if (!LITTLEFS.begin(false))
	{
		log_e("LITTLEFS Mount Failed. STOPPED.");
		while (1)
			delay(1);
	}
	else
	{
		log_i("LITTLEFS system mounted SUCCESSFUL.");
	}

	// Start the display so we can show connection/hardware errors on it
	// From here we can use funtion displayTrackArtist(<message>)
	initDisplay();

	// VS1053 MP3 decoder. If we don't get a response continue anyway.
	log_d("Starting Audio Player.");
	displayTrackArtist("Starting Audio Player.");

	pinMode(VS1053_RST, OUTPUT);
	digitalWrite(VS1053_RST, LOW);
	delay(100);
	digitalWrite(VS1053_RST, HIGH);

	while (!player.begin())
	{
		// Reset the entire unit
		log_w("VS1053 did not start - resetting VS1053B unit");
		//ESP.restart();
		digitalWrite(VS1053_RST, LOW);
		delay(100);
		digitalWrite(VS1053_RST, HIGH);
		delay(1000);
	};
	// Note: player.begin can sometimes hang.
	// FIXME:Needs to be looked at (perhaps a later version of the library fixes this)
	// IN PROGRESS: New library used but still hangs occasionally

	// Mute the sound temporarily
	player.setVolume(0);

	// You MIGHT have to set the VS1053 to MP3 mode. No harm done if not required!
	log_d("Switch player to MP3 mode");
	displayTrackArtist("Switching to MP3 mode.");
	player.switchToMp3Mode();
	delay(250);

	// Set the equivalent of "Loudness" (increased bass & treble)
	// TODO: couldn't get this to work without a new function so needs looking at again
	// This works best if the volume is NOT set to 100%.

	// 	Name 		Bits 	Description
	// ST_AMPLITUDE 15:12 	Treble Control in 1.5 dB steps (-8..7, 0 = off)
	// ST_FREQLIMIT 11:8 	Lower limit frequency in 1000 Hz steps (1..15)
	char trebleCutOff = 2; // kHz cutoff
	char trebleBoost = 5 << 4 | trebleCutOff;
	log_d("Treble boost expected to be: %d, is: %d", (5 * 16) + trebleCutOff, trebleBoost);

	// SB_AMPLITUDE 7:4 	Bass Enhancement in 1 dB steps (0..15, 0 = off)
	// SB_FREQLIMIT 3:0 	Lower limit frequency in 10 Hz steps (2..15)
	char bassCutOff = 10; // times 10 = 100Hz cutoff
	char bassBoost = 8 << 4 | bassCutOff;
	log_d("Bass boost expected to be: %d, is: %d", (8 * 16) + bassCutOff, bassBoost);

	uint16_t SCI_BASS = trebleBoost << 8 | bassBoost;
	player.setAudio(SCI_BASS);

	// Set the volume here to previous value (or default if none)
	userVolume = preferences.getUInt("Volume", 95);
	log_v("Previous volume was %d", userVolume);
	targetVolume = userVolume;

	player.setVolume(targetVolume);
	displayTrackArtist("Playing welcome message.");

	// Some sort of startup message (I just recorded this using
	// https://onlinetonegenerator.com/voice-generator.html)
	File file = LITTLEFS.open("/Intro.mp3");
	if (file)
	{
		uint8_t audioBuffer[32] __attribute__((aligned(4)));
		while (file.available())
		{
			int bytesRead = file.read(audioBuffer, 32);
			player.playChunk(audioBuffer, bytesRead);
		}
		file.close();
	}
	else
	{
		log_w("Unable to open greetings file '/Intro.mp3'");
		displayTrackArtist("Unable to open file '/Intro.mp3'");
	}

	// Mute sound (will be faded up on station play)
	player.setVolume(0);

	// Read the radio station list
	displayTrackArtist("Reading station list.");
	stationSelectSetup();

	// Start WiFi
	char buffer[50] = "";
	ssid = getSSID();
	wifiPassword = getWiFiPassword();
	sprintf(buffer, "Connecting to WiFi - %s", ssid.c_str());
	displayTrackArtist(buffer);

	while (wiFiDisconnected)
	{
		connectToWifi();
		if (wiFiDisconnected)
		{
			// TODO: Print message to TFT otherwise won't know it's failed
			log_w("Unable to connect to WiFi network: '%s'", ssid.c_str());
			delay(500);
		}
	}

	// Whether we want MetaData or not. Connect the pin to GND to skip METADATA.
	METADATA = digitalRead(ICYDATAPIN) == HIGH;

	// Get the station number that was previously playing
	currStnNo = preferences.getUInt("currStnNo", 0);
	if (currStnNo > radioStation.size())
	{
		currStnNo = 0;
	}
	prevStnNo = currStnNo;

	// FIXME: Is this required here (in setup)? Done in loop()
	// Set the station listing page number so when we open that it is on
	// the correct page for the current station (used in stationSelectHelper.h, listStationsOnScreen)
	currPage = currStnNo / MAX_STATIONS_PER_SCREEN;
	log_d("Current station %d appears on page %d", currStnNo, currPage);

	// Connect to that station
	displayTrackArtist("Connect to previous station");
	log_d("Current station number: %u", currStnNo);

	// Station list count might have reduced
	if (currStnNo > radioStation.size() - 1)
	{
		currStnNo = 0;

		// Store (new) current station in EEPROM
		preferences.putUInt("currStnNo", currStnNo);
		log_d("New station now stored: %u", nextStnNo);
	}

	if (!stationConnect(currStnNo))
	{
		connectedToStation = false;
		displayTrackArtist("Cannot connect to this station.");
	}
	else
	{
		connectedToStation = true;
	}

	// Set screen brightness to previous level
	ledcSetup(0, 5000, 8);
	ledcAttachPin(TFT_BL, 0);

	// 0 (off) to 255(bright) duty cycle
	prevTFTBright = preferences.getUInt("Bright", 255);

	// If the screen brightness is too low, restore to minimum level
	if (prevTFTBright < 20)
	{
		log_w("TFT Brightness level increased from %d to %d", prevTFTBright, 20);
		prevTFTBright = 20;
	};

	ledcWrite(0, prevTFTBright);
	log_d("Restored screen brightness to %d", prevTFTBright);

	// We need to set up independent task that plays the music from the circular buffer
	taskSetup();

	//Now how much SRAM free (heap memory)
	log_d("Free memory: %d bytes", ESP.getFreeHeap());

	/*
	getRedirectedStationInfo("http://dsldevice.lan/?org_url=http://stream.antenne1.de%2fa1stg%2flivestream1%2eaac&help_wizard=1",1);
	getRedirectedStationInfo("http://stream.antenne1.de%2fa1stg%2flivestream1%2eaac&help_wizard=1",2);
	getRedirectedStationInfo("http://stream.bbc.co.uk/myStream.aac",3);
	
	while(1);
	*/
}

// ==================================================================================
// loop     loop     loop     loop    loop     loop     loop     loop    loop    loop
// ==================================================================================
void loop()
{
	// If there is data available, try and get it
	readHttpStream();

	// So how many bytes have we got in the buffer (should hover around 90%)
	if (currDisplayScreen == HOME)
	{
		drawBufferLevel(circBuffer.available());

		// Has CHANGE STATION button been pressed?
		checkForStationChange();

		//Volume / Mute/ Unmute activate
		getSpkrButtonPress();

		// Screen Brightness activate
		getBulbButtonPress();

		// Settings Button (bass, treble etc)
		getSettingsBtn();

		// Screen brightness or volume change
		if (bulbStatus == BTN_ACTIVE || (spkrStatus == BTN_ACTIVE && !isMutedState))
		{
			getPlusButtonPress();
			getMinusButtonPress();
		}

		// Fade in the music if not already at max (if not muted)
		if (connectedToStation && canChangeStn && delaySinceStationChange())
		{
			int playerVolume = player.getVolume();
			if (playerVolume != targetVolume)
			{
				//log_v("Player volume:%d, target volume:%d",player.getVolume(),targetVolume);
				fadeInMusic(playerVolume);
			}
		}

#if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
		// In debug mode it's useful to have the physical NEXT button working
		checkForNextButton();

		static unsigned long prevMillisFreeMem = 0;

		//How much SRAM free (heap memory) every few minutes
		if (millis() - prevMillisFreeMem > 300000)
		{
			prevMillisFreeMem = millis();
			log_d("Free memory: %d bytes", ESP.getFreeHeap());
		}
#endif
	}
}

// If there is data ready and we have room in the circular buffer, read the mp3/aac stream
void readHttpStream()
{
	// Low buffer monitoring
	static unsigned long circBufferLow = millis();
	static uint8_t lowCircBuffCnt = 0;

	// Data to read from mainBuffer?
	int bytesAvailableFromStream = client.available();
	if (bytesAvailableFromStream > 0 && connectedToStation)
	{
		populateRingBuffer();

		// If we've read all data between metadata bytes, check to see if there is track title to read
		if (bytesUntilmetaData == 0 && METADATA)
		{
			// TODO: if cant read valid metadata we need to reconnect to the station to resync
			if (!readMetaData())
			{
				// Reconnect to the current station but don't clear the circ buffer the data is still good
				if (!stationConnect(currStnNo, false))
				{
					// If we can't connect to station check wifi
					if (!client.connected())
					{
						connectToWifi();
						connectedToStation = false;
					}
					else
					{
						// Set global flag that we are not connected to a radio station. The HOME
						// screen will be displayed with an advisory message as to what went wrong.
						connectedToStation = false;
					}

					log_d("No longer connected to a radio station.");
				};
			}

			// reset byte count for next bit of metadata
			bytesUntilmetaData = metaDataInterval;
		}
	}
	else
	{
		// Sometimes we get randomly disconnected from WiFi BUG: Why?
		if (WiFi.status() != WL_CONNECTED)
		{
			// TODO: if client suddenly not connected we will have to reconnect
			log_w("WiFi client not connected.");
			connectToWifi();

			// Attempt reconnection to station
			connectedToStation = false;
			//stationConnect(currStnNo, false);
			changeStation(currStnNo);
		}

		// Perhaps we are no longer connected to a URL stream
		static unsigned long prevMillisConnected = 0;
		if (!connectedToStation)
		{
			if (millis() - prevMillisConnected > THIRTYSECONDS)
			{
				prevMillisConnected = millis();
				log_w("Not connected to a radio station, attempting to reconnect.");

				// Clear down other flags/count that are monitoring connection/streaming success
				lowCircBuffCnt = 0;
				circBufferLow = millis();

				// Attempt reconnection to station
				//stationConnect(currStnNo, false);
				changeStation(currStnNo, true);
			}
		}
	}

	// Nothing available from the WiFi stream? Not unusual, apparently.
	// static unsigned long prevMillisClient = 0;
	// if (millis() - prevMillisClient > FIVESECONDS)
	// {
	// 	prevMillisClient = millis();
	// 	log_v("WiFi Client: %d bytes available.", bytesAvailableFromStream);
	// }

	// RSB If the circular buffer collapses and doesn't recover, we must retune, might be a redirect
	if (bufferPerCent < 5)
	{
		// If buffer still low after timeout, log this
		if (millis() - circBufferLow > LOWBUFFER_SECONDS)
		{
			// We re-tune here if it goes on too long
			log_d("Low buffer: %d%%, occurence %d", bufferPerCent, ++lowCircBuffCnt);

			// Reset low buffer start time
			circBufferLow = millis();

			// Retune if there is no recovery
			if (lowCircBuffCnt >= LOWBUFFER_RECOVERYCOUNT_MAX)
			{
				log_w("Low buffer: atempting to reconnect to current station");
				lowCircBuffCnt = 0;

				// Whether successful reconnection or not, error flags and counts will be reset when buffer > 5%
				// We won't get to this point again for at least LOWBUFFER_SECONDS anyway, plenty of time for
				// the buffer to fill, if it's going to
				changeStation(currStnNo, true);
			}
		}
	}
	else
	{
		// Clear previous low buffer count, now recovered
		if (lowCircBuffCnt > 0)
		{
			log_d("Low buffer: recovered to %d%%", bufferPerCent);
			lowCircBuffCnt = 0;
		}
	}
}

// Populate ring buffer with streaming data (if we have some - this routine only called if bytes available in stream)
// Must NOT be called directly, only via readHttpStream
void populateRingBuffer()
{
	// Signed because we might get -1 returned (which means: couldn't read)
	signed int bytesReadFromStream = 0;

	// If we can't read bytes from the stream, keep track so we can reconnect
	static int noBytesReadCount = 0;

	// Room in the ring buffer for (up to) X bytes?
	if (circBuffer.room() >= streamingCharsMax)
	{
		// Read either the maximum available (max 32) or the number of bytes to the next meta data interval
		int expectedBytesFromStream = min(streamingCharsMax, METADATA ? (int)bytesUntilmetaData : streamingCharsMax);

		// If this is chunked data (transfer-coding = "chunked" appears as a header) then ensure we skip the
		// metadata around the chunk. First N bytes (up to CR/LF gives chunk size, all in ASCII hex format)
		if (isChunked)
		{
			// Remove terminating CRLF after the real chunk data
			if (bytesUntilNextChunk == 2)
			{
				//log_v("Discarding final CRLF pair from chunk.");
				while (client.readBytes(chunkSizeByte, 1) == 0)
				{
					log_v("Couldn't read final chunk byte #1 - retrying");
				};

				while (client.readBytes(chunkSizeByte, 1) == 0)
				{
					log_v("Couldn't read final chunk byte #2 - retrying");
				};

				// Reset the bytes remaining in the chunk
				bytesUntilNextChunk = 0;
			}

			// If this is the very first chunk or there are no bytes left in the chunk, prepare to read another chunk
			if (bytesUntilNextChunk == 0)
			{
				// Extract chunk size
				//log_v("Extracting chunk size from stream");

				// Initialise buffer that holds the chunk size
				memset(chunkSizeBuffer, 0, 20);

				// Read first N bytes until CR/LF pair to get chunk size
				uint8_t chunkSizeCount = 0;

				while (1)
				{
					// Read one byte at a time from the stream until CRLF pair
					while (client.readBytes(chunkSizeByte, 1) == 0)
					{
						// This is the same "error" as not being able to read client stream - not unusual
						// log_v("Couldn't read a chunk byte - retrying");
					};

					// If we have a CRLF pair then we have extract the ASCII size of the chunk
					if (chunkSizeByte[0] == 13 && client.peek() == 10)
					{
						// We have completed reading the chunk size into the buffer
						// log_v("Chunk size buffer (excl CRLF) %s", chunkSizeBuffer);

						// Read (and discard) the next LF char
						while (client.readBytes(chunkSizeByte, 1) == 0)
						{
							log_v("Couldn't read final LF chunk byte - retrying");
						};

						// Convert the ASCII hex to an integer (eg "1" "0" "0" "0" is not 1000 it's 4096)
						char *pEnd;
						long realChunkSize = strtol(chunkSizeBuffer, &pEnd, 16);
						//log_v("Chunk size buffer (excl CRLF) %d", realChunkSize);

						// Add two to the count for the terminating CRLF pair. This makes it
						// easier (for me) to detect the end of a chunk except for the first chunk
						bytesUntilNextChunk = realChunkSize + 2;

						// Exit this loop
						break;
					}
					else
					{
						// Put the character into the chunk size buffer
						//log_v("Chunk size buffer (so far): %s", chunkSizeBuffer);
						chunkSizeBuffer[chunkSizeCount++] = chunkSizeByte[0];
					}
				}
			}
		}
		if (isChunked)
		{
			expectedBytesFromStream = min(expectedBytesFromStream, (int)bytesUntilNextChunk - 2);
		}

		bytesReadFromStream = client.read((uint8_t *)readBuffer, expectedBytesFromStream);

		// If we get -1 here it means that nothing could be read from the stream
		// TODO: find out why this might be. Remote server not streaming?
		if (bytesReadFromStream > 0)
		{
			// Subtract the number of bytes read from the chunk
			if (isChunked)
			{
				bytesUntilNextChunk -= bytesReadFromStream;

				//if (bytesUntilNextChunk < 20)
				//	log_v("Bytes until next chunk: %d", bytesUntilNextChunk);
			}

			// Add them to the circular buffer
			circBuffer.write(readBuffer, bytesReadFromStream);

			// If we didn't read the amount we "expected" debug that here
			if (bytesReadFromStream < expectedBytesFromStream && expectedBytesFromStream != streamingCharsMax)
			{
				log_v("Only %d bytes (wanted %d) read from stream.", bytesReadFromStream, expectedBytesFromStream);
			}

			// Subtract bytes actually read from incoming http data stream from the bytesUntilmetaData
			bytesUntilmetaData -= bytesReadFromStream;

			// Reset read error flag
			if (noBytesReadCount > 0)
			{
				if (noBytesReadCount > 10)
					log_v("Resetting stream zero-bytes read count of %d to zero", noBytesReadCount);
				noBytesReadCount = 0;
			}
		}
		else
		{
			// Only inform infrequently as this is very common and is mostly reset on the next read attempt
			if (++noBytesReadCount % 10 == 0)
			{
				// Report error reading from HTTP stream. This can happen often and does
				// not necessarily indicate a problem. However, if it continues it becomes one.
				log_d("Unable to read stream, occurrence: %d", noBytesReadCount);
			}

			// If the threshold is exceeded (when no bytes were read) reconnect to the current station
			if (noBytesReadCount > 10 && bufferPerCent < 5)
			{
				log_d("Unable to read stream and buffer at %d bytes - reconnect to current station");

				// FIXME: Not the place to do this but for quick fix might be OK
				// Do not clear the buffer - if there is data there it is still good
				if (stationConnect(currStnNo, false))
				{
					// We reconnected so reset the problem counter
					log_d("Reconnection SUCCESSFUL, resetting error flag");
					noBytesReadCount = 0;
				}
				else
				{
					log_d("Reconnection UNSUCCESSFUL, will retry");
				};
			}
		}
	}
}

// Connect to the station list number
bool stationConnect(int stationNo, bool clearBuffer)
{
	log_d("-----------------------------------");
	log_d("     Connecting to station %d", stationNo);
	log_d("-----------------------------------");
	log_d("HOST: %s", radioStation[stationNo].host.c_str());
	log_d("PATH: %s", radioStation[stationNo].path.c_str());
	log_d("PORT: %d", radioStation[stationNo].port);
	log_d("NAME: %s", radioStation[stationNo].friendlyName.c_str());
	log_d("DATA: %s", radioStation[stationNo].useMetaData ? "Yes" : "No");
	if (redirected)
	{
		log_i("-This station has been redirected to a new URL-");
	}

	// Clear down any previous metadata info
	currstreamArtistTitle = "";

	// Clear any chunked transfer-enocding. Will be specified in a header
	isChunked = false;
	bytesUntilNextChunk = 0;

	// If we are here because of corrupt metaData, don't do anything to existing data
	if (clearBuffer)
	{
		// Kill the sound whilst we retune
		fadeOutMusic();
		log_v("Player volume now: %d", player.getVolume());

		// Clear down the streaming buffer and the RX stream
		log_d("Circ buffer FLUSHED");
		circBuffer.flush();

		// Flag to indicate we need to buffer data before allowing player to stream audio
		canPlayMusicFromBuffer = false;
	}

	//How much SRAM free (heap memory)
	log_i("Free memory: %d bytes", ESP.getFreeHeap());

	// Determine whether we want ICY metadata
	METADATA = digitalRead(ICYDATAPIN) == HIGH;

	// For THIS radio station have we FORCED meta data to be ignored?
	METADATA = METADATA ? radioStation[stationNo].useMetaData : METADATA;
	if (!radioStation[stationNo].useMetaData)
	{
		log_d("METADATA ignored for this radio station.");
	}

	// Set the metadataInterval value to zero so we can detect that we found a valid one
	metaDataInterval = 0;

	// Clear down any screen info and display station name (with redirection marker)
	if (radioStation[stationNo].friendlyName.find(" [R]", 0) != std::string::npos)
		radioStation[stationNo].friendlyName += redirected ? " [R]" : "";

	displayStationName(radioStation[stationNo].friendlyName);
	displayTrackArtist((char *)"Connecting...");
	drawBufferLevel(0, true);

	// We try a few times to connect to the station
	bool connected = false;
	int connectAttempt = 0;

	while (!connected && connectAttempt < 5)
	{
		if (redirected)
		{
			log_d("REDIRECTED URL DETECTED FOR STATION %d", stationNo);
		}

		connectAttempt++;
		log_d("Host: %s Port:%d", radioStation[stationNo].host.c_str(), (int)radioStation[stationNo].port);

		// Connect to the redirected URL
		if (client.connect(radioStation[stationNo].host.c_str(), radioStation[stationNo].port))
		{
			connected = true;
		}
	}

	// If we could not connect (eg bad URL) just exit
	if (!connected)
	{
		log_w("Could not connect to %s", radioStation[stationNo].host.c_str());
		displayTrackArtist("Could not connect to this station");
		return false;
	}

	log_d(
		"Connected to %s (%s%s)",
		radioStation[stationNo].host.c_str(), radioStation[stationNo].friendlyName.c_str(),
		redirected ? " - redirected" : "");

	displayTrackArtist("Connected. Reading stream...");

	// Get the data stream plus any metadata (eg station name, track info between songs / ads)
	// TODO: Allow retries here (BBC Radio 4 very finicky before streaming).
	// We might also get a redirection URL given back.
	log_i(
		"Getting data from %s (%s Metadata)",
		radioStation[stationNo].path.c_str(),
		(METADATA ? "WITH" : "WITHOUT"));

	client.print(
		String("GET ") + radioStation[stationNo].path.c_str() + " HTTP/1.1\r\n" +
		"Host: " + radioStation[stationNo].host.c_str() + "\r\n" +
		(METADATA ? "Icy-MetaData:1\r\n" : "") +
		"Connection: close\r\n\r\n");

	// Give the client a chance to connect
	log_d("Waiting for header data");
	int retryCnt = 30;
	while (client.available() == 0 && --retryCnt > 0)
	{
		delay(100);
	}

	if (client.available() < 1)
	{
		displayTrackArtist("No data stream could be found");
		return false;
	}

	// Keep reading until we read two LF bytes in a row.
	//
	// The ICY (I Can Yell, a precursor to Shoutcast) format:
	// In the http response we can look for icy-metaint: XXXX to tell us how far apart
	// the header information (in bytes) is sent.

	// Process all responses until we run out of header info (blank header)
	while (client.available())
	{
		// Delimiter char is not included in data returned
		String responseLine = client.readStringUntil('\n');

		// If the header is not empty process it
		log_d("HEADER: %s", responseLine.c_str());

		if (responseLine.indexOf("200 OK") > 0)
		{
			// TODO: we really should check we have a response of 200 before anything else
			log_d("200 - OK response");
			redirected = false;
			continue;
		}

		// HTTP/1.0 302 Redirect
		if (responseLine.indexOf("302 Redirect") > 0)
		{
			// Redirected to another URL
			log_d("302 Redirect detected");
			redirected = true;
			continue;
		}

		// If we have an EMPTY header (or just a CR) that means we had two linefeeds
		if (responseLine[0] == (uint8_t)13 || responseLine == "")
		{
			break;
		}

		// Critical value for this whole sketch to work: bytes between "frames"
		// Sometimes you can't get this first time round so we just reconnect
		// (Actually it's only BCC Radio 4 that doesn't always give this out)
		if (responseLine.startsWith("icy-metaint"))
		{
			metaDataInterval = responseLine.substring(12).toInt();
			log_d("NEW Metadata Interval:%d", metaDataInterval);
			continue;
		}

		// Has this stream got 'transfer encoding' = 'chunked' (for http/1.1 only)?
		if (responseLine.startsWith("Transfer-Encoding: chunked"))
		{
			isChunked = true;
			log_d("Current station uses tranfer-encoding: chunked");
			continue;
		}

		// The bit rate of the transmission (FYI) eye candy
		if (responseLine.startsWith("icy-br:"))
		{
			bitRate = responseLine.substring(7).toInt();
			log_d("Bit rate:%d", bitRate);
			continue;
		}

		// The URL we used has been redirected?
		if (responseLine.startsWith("Location: http://"))
		{
			log_v("Redirection detected: %s", responseLine.c_str());
			getRedirectedStationInfo(responseLine, stationNo);
			redirected = true;
			return false;
		}
	}

	// If we didn't find required metaDataInterval value in the headers, abort this connection
	if (metaDataInterval == 0 && METADATA)
	{
		log_e("NO METADATA INTERVAL DETECTED");
		if (currDisplayScreen == HOME)
			displayTrackArtist("No MetaData interval found");

		// TODO: If this happens assume that the stationList.txt is wrong and don't get metadata
		radioStation[stationNo].useMetaData = 0;
		return false;
	}

	// Update the count of bytes until the next metadata interval (used in loop) and exit
	bytesUntilmetaData = metaDataInterval;

	// Restore music volume (if not muted)
	targetVolume = isMutedState ? 1 : userVolume;

	// Set time so fade in of music is delayed a bit
	timeAtStationChange = millis();

	// All done here
	return true;
}

// LITTLEFS card reader (done ONCE in setup)
// Format of data is:
//          #comment line
//          <KEY><DATA>
std::string readLITTLEFSInfo(char *itemRequired)
{
	char startMarker = '<';
	char endMarker = '>';
	char *receivedChars = new char[32];
	int charCnt = 0;
	char data;
	bool foundKey = false;

	log_v("Looking for key '%s'", itemRequired);

	// Get a handle to the file
	File configFile = LITTLEFS.open("/WiFiSecrets.txt", FILE_READ);
	if (!configFile)
	{
		// TODO: Display error on screen
		log_e("Unable to open file /WiFiSecrets.txt");
		while (1)
			;
	}

	// Look for the required key
	while (configFile.available())
	{
		charCnt = 0;

		// Read until start marker found
		while (configFile.available() && configFile.peek() != startMarker)
		{
			// Do nothing, ignore spurious chars
			data = configFile.read();
			//log_d("Throwing away preMarker:");
			//Serial.println(data);
		}

		// If EOF this is an error
		if (!configFile.available())
		{
			// Abort - no further data
			continue;
		}

		// Throw away startMarker char
		configFile.read();

		// Read all following characters as the data (key or value)
		while (configFile.available() && configFile.peek() != endMarker)
		{
			data = configFile.read();
			receivedChars[charCnt] = data;
			charCnt++;
		}

		// Terminate string
		receivedChars[charCnt] = '\0';

		// If we previously found the matching key then return the value
		if (foundKey)
			break;

		//log_d("Found: '%s'", receivedChars);
		if (strcmp(receivedChars, itemRequired) == 0)
		{
			//Serial.println("Found matching key - next string will be returned");
			foundKey = true;
		}
	}

	// Terminate file
	configFile.close();

	// Did we find anything
	log_d("LITTLEFS parameter '%s'", itemRequired);
	if (charCnt == 0)
	{
		log_d("' not found.");
		return "";
	}
	else
	{
		// WARNING This will display the WiFi password to the log
		//log_i("': '%s'", receivedChars);
	}

	return receivedChars;
}

// We can tune to a new station or just tune into the current again if there's a problem
bool changeStation(int8_t nextStnNo, bool alwaysRetune)
{
	log_i("--------------------------------------");
	log_i("    Change Station: %s", radioStation[nextStnNo].friendlyName.c_str());
	log_i("--------------------------------------");

	// Make button inactive
	canChangeStn = false;

	// Reset any redirection flag
	redirected = false;

	// Whether connected to this station or not, update the variable otherwise
	// we would 'stick' at old station
	// TODO: No longer true, only applied to the single NEXT mechanical button method
	currStnNo = nextStnNo;

	log_d("Current radio station:%d, new radio station:%d", prevStnNo, nextStnNo);
	if (prevStnNo != nextStnNo || alwaysRetune)
	{
		prevStnNo = nextStnNo;

		// Now actually connect to the new (or same) URL for the station. If we are
		// forcing a reconnect to the same station (due to problem) don't clear buffer
		if (!stationConnect(nextStnNo, !alwaysRetune))
		{
			if (!client.connected())
			{
				connectToWifi();
			}
			else
			{
				canChangeStn = true;
				return false;
			}
		};

		// Store (new) current station in EEPROM
		preferences.putUInt("currStnNo", nextStnNo);
		log_d("Current station now stored: %u", nextStnNo);
	}
	else
	{
		// Reinstate the HOME screen
		displayStationName(radioStation[currStnNo].friendlyName);
		displayTrackArtist(toTitle(currstreamArtistTitle));
		drawBufferLevel(circBuffer.available(), true);
		log_i("Same station selected - no further action required");
		log_i("Room in circular buffer: %d", circBuffer.room());
	}

	// Ability to change station active again
	canChangeStn = true;
	return true;
}

/*
	Every n bytes of stream data there is ONE byte (x 16)that determines how much metadata there is
	that describes the track/artis. Some radio stations use this to display other messages,
	such as presenter name (All Request Lunch with Lee Hynes), messages to the consumer (Please
	Wear Your Face Mask) and competition telephone numbers(Text Now 087 3737956), to name but a few.

	Some stations also send out a URL for the current station - not for listening but for 
	extracting further (JSON) data that gives links to artist album cover art, for example.
	TODO: extract JSON data for album art (and anything else that might be useful).

	To see all this in action tune to the 64Kb Irish station, South East Radio on URL: 
	http://stream.btsstream.com:8000/seaac
	Note: you may get quite a few "corrupt" metadata characters initially, but this settles down
	over time. No idea why. Perhaps they are Irish language characters?
	FIXME: Find out why this station gives these corrupt characters initially 
*/
inline bool readMetaData()
{
	// The first byte is going to be the length of the metadata
	int metaDataLength = client.read();

	// Usually there is none as track/artist info is only updated when it changes
	// It may also return the station URL (not necessarily the same as we are using) containing JSON.
	// Example:
	//  'StreamTitle='Love Is The Drug - Roxy Music';StreamUrl='https://listenapi.planetradio.co.uk/api9/eventdata/62247302';'
	if (metaDataLength < 1) // includes -1
	{
		// Warning sends out about 4 times per second!
		//log_v("No metadata to read.");
		return "";
	}

	// The actual length is 16 times bigger to allow from 16 up to 4080 bytes (255 * 16) of metadata
	metaDataLength = (metaDataLength * 16);
	log_v("Metadata block size: %d", metaDataLength);

	// Wait for the entire MetaData to become available in the stream
	log_v("Waiting for METADATA");
	while (client.available() < metaDataLength)
	{
		delayMicroseconds(1);
	}

	// Temporary buffer for the metadata
	char metaDataBuffer[metaDataLength + 1];

	// Initialise this temp buffer
	memset(metaDataBuffer, 0, metaDataLength + 1);

	// Populate it from the internet stream
	client.readBytes((char *)metaDataBuffer, metaDataLength);
	log_v("MetaData: %s", metaDataBuffer);

	for (auto cnt = 0; cnt < metaDataLength; cnt++)
	{
		if (metaDataBuffer[cnt] > 0 && metaDataBuffer[cnt] < 8)
		{
			log_w("Corrupt METADATA found:%02X, length:%d, retuning...", metaDataBuffer[cnt], metaDataLength);

			// Terminate the string right after the corrupt char so we can print it
			metaDataBuffer[cnt + 1] = '\0';
			log_v("Metadata read (until corrupt character):'%s'", metaDataBuffer);
			return false;
		}
	}

	// Extract track Title/Artist from this string
	char *startTrack = NULL;
	char *endTrack = NULL;
	std::string streamArtistTitle = "";

	startTrack = strstr(metaDataBuffer, "StreamTitle");
	if (startTrack != NULL)
	{
		// We have found the streamtitle so just skip over "StreamTitle="
		startTrack += 12;

		// Now look for the end marker
		endTrack = strstr(startTrack, ";");
		if (endTrack == NULL)
		{
			// No end (very weird), so just set it as the string length
			endTrack = (char *)startTrack + strlen(startTrack);
		}

		// There might be an opening and closing quote so skip over those (reduce data width) too
		if (startTrack[0] == '\'')
		{
			startTrack += 1;
			endTrack -= 1;
		}

		// We MUST terminate the 'string' (character array) with a null to denote the end
		endTrack[0] = '\0';

		// Extract the data by adjusting pointers
		ptrdiff_t startIdx = startTrack - metaDataBuffer;
		ptrdiff_t endIdx = endTrack - metaDataBuffer;
		std::string streamInfo(metaDataBuffer, startIdx, endIdx);
		streamArtistTitle = streamInfo;

		// Debug only if there is something to see
		if (streamArtistTitle != "")
		{
			log_v("%s", streamArtistTitle.c_str());
		}

		// Always output the Artist/Track information even if just to clear it from screen,
		// but not if we're not on the HOME screen (eg changing channels) or it hasn't changed
		// as some broadcasters (eg Classic FM) send this down every 500mS!
		if (currstreamArtistTitle == streamArtistTitle && streamArtistTitle != "")
		{
			log_v("New and old metadata info IDENTICAL");
		}
		else
		{
			log_v("Previous: '%s'", currstreamArtistTitle.c_str());
			log_v("New info: '%s'", streamArtistTitle.c_str());

			if (currDisplayScreen == HOME)
			{
				displayTrackArtist(toTitle(streamArtistTitle));
			}
		}

		// Update the new Artist/Title string from metadata
		currstreamArtistTitle = streamArtistTitle;
	}

	// All done
	return true;
}

// Our streaming station has been 'redirected' to another URL
// The header will look like: Location: http://<new host / path>[:<port>]
// Location: http://dsldevice.lan/?org_url=http://stream.antenne1.de%2fa1stg%2flivestream1%2eaac&help_wizard=1
void getRedirectedStationInfo(String header, int currStationNo)
{
	log_i("--------------------------------------");
	log_i(" Extracting redirection information");
	log_i("--------------------------------------");

	// Placeholders for the new host/path
	std::string redirectedHost = "";
	std::string redirectedPath = "";

	// We'll assume the port is 80 unless we find one in the host name
	int redirectedPort = 80;

	// Find the LAST occurrence of the http:// header so we extract the correct URL
	size_t last_http = header.lastIndexOf("http://");

	// Skip the http:// bit
	header = header.substring(last_http + 7);
	log_d("Redirected encoded URL: '%s'", header.c_str());

	// Unencode the URL into standard ASCII characters
	std::string oldHeader = header.c_str();
	std::string decodedURL = urlDecode(oldHeader);
	header = decodedURL.c_str();
	log_d("Redirected decoded URL: '%s'", header.c_str());

	// Remove any parameters (start with &)
	int8_t ampersand = header.indexOf('&');
	if (ampersand > 0)
	{
		log_d("Addition parms from char %d and discarded: %s", ampersand, header.substring(ampersand).c_str());
		header = header.substring(0, ampersand);
		log_v("Redirected Final URL: %s", header.c_str());
	}

	// Split the header into host and path constituents
	int pathDelimiter = header.indexOf("/");
	if (pathDelimiter > 0)
	{
		redirectedPath = header.substring(pathDelimiter).c_str();
		redirectedHost = header.substring(0, pathDelimiter).c_str();
	}

	// Look to split host into host and port number
	// Example: stream/myradio.de:8080
	int portDelimter = header.indexOf(":");
	if (portDelimter > 0)
	{
		redirectedPort = header.substring(portDelimter + 1).toInt();

		// Adjust the host name to exclude the port information
		redirectedHost = redirectedHost.substr(0, portDelimter);
	}

	// Just overwrite the current entry for this station (reverts on reboot)
	log_w("New address: %s:%d%s", redirectedHost.c_str(), redirectedPort, redirectedPath.c_str());
	radioStation[currStationNo].host = redirectedHost.c_str();
	radioStation[currStationNo].path = redirectedPath.c_str();
	radioStation[currStationNo].port = redirectedPort;

	// Set previous station number to artificial number to force reconnect
	prevStnNo = 999;
	return;
}

// Change station screen button pressed?
void checkForStationChange()
{
	// Only allow this function to run infrequently or we skip music
	static unsigned long prevMillis = millis();

	// Every so often
	if (millis() - prevMillis > 100)
	{
		prevMillis = millis();

		// Was the screen touched (and is it a valid touch)
		if (!digitalRead(tftTouchedPin) && canChangeStn)
		{
			if (getStnChangeButtonPress())
			{
				// Set the current screen with the current station on it
				currDisplayScreen = STNSELECT;

				// Set the station listing page number so when we open that it is on
				// the correct page for the current station (used in stationSelectHelper.h, listStationsOnScreen)
				currPage = currStnNo / MAX_STATIONS_PER_SCREEN;
				log_d("Current station %d appears on page %d", currStnNo, currPage);

				// Display the (current) page of stations plus Next/Prev buttons
				initStationSelect();

				while (1)
				{
					// Get the station number that was pressed
					int nextStationNo = getStationListPress();

					// If we selected a station, change to it
					if (nextStationNo > -1)
					{
						// No point in allowing the ring buffer to be populated now; note that
						// if we have selected the SAME station, it will just continue with the
						// same ring buffer (doesn't flush)
						connectedToStation = false;

						log_d("Station %d received in loop", nextStationNo);
						buttonPressPending = true;

						// Redraw the home screen (banner, station name etc)
						currDisplayScreen = HOME;
						setupDisplayModule(false);

						// Change station (or keep current one)
						if (changeStation(nextStationNo))
						{
							connectedToStation = true;
						}
						else
						{
							// If we were redirected to new URL give it another shot
							if (redirected)
							{
								if (changeStation(nextStationNo))
								{
									connectedToStation = true;
								}
							}
						};

						// We're done here, connected or fail
						break;
					}

					// Are we trying to page the list of stations?
					stationNextPrevButtonPressed();

					// Get more data into circular buffer before we run out
					doCriticalTasks();
				}
			}
		}
	}
}

// Fade out the music rather than setting to 0 - stops the "click"
void fadeOutMusic()
{
	log_d("Fading music OUT");
	int playerVolume = player.getVolume();

	// Reduce volume to +1 (stops annoying click at zero)
	for (auto cnt = playerVolume; cnt > 0; cnt -= cnt >= 5 ? 5 : 1)
	{
		player.setVolume(cnt);
		log_v("Reducing player volume of %d to %d", playerVolume, cnt);

		// Update visual indicator
		drawPercentageLine(VOLUME);
		delay(2);
	}
}

// If the music stream is played immediately at 100% we get jitter. Follow Amazon's example
// and fade it up over a few seconds or so
void fadeInMusic(int playerVolume)
{
	static unsigned long prevMillis = millis();

	if (millis() - prevMillis > 10)
	{
		prevMillis = millis();

		if (playerVolume < targetVolume)
		{
			player.setVolume(playerVolume + (playerVolume <= 50 ? 10 : 1));

			if (playerVolume == 1)
			{
				log_d("Increasing music volume");
			}
		}
		else if (playerVolume > targetVolume)
		{
			player.setVolume(playerVolume - (playerVolume >= 50 ? (playerVolume - targetVolume < 5 ? 1 : 5) : 1));

			if (playerVolume == 1)
			{
				log_d("Reducing music volume");
			}
		}

		// Update visual indicator
		drawPercentageLine(VOLUME);
	}
}

// Prevents fade in of music until minium time since station change to avoid glitchy sound
bool delaySinceStationChange()
{
	if (millis() - timeAtStationChange > MIN_DELAY_AFTER_STATION_CHANGE)
	{
		return true;
	}

	return false;
}

// "Next" station hardware button pressed? Useful for debugging
#if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
void checkForNextButton()
{
	// Only allow this function to run infrequently or we skip music
	static unsigned long prevMillis = millis();

	// Physical button(s) go LOW when active
	if (millis() - prevMillis > 100)
	{
		prevMillis = millis();

		// First check for physical button press
		if (!digitalRead(stnChangePin) && canChangeStn)
		{
			changeStation(++currStnNo > radioStation.size() - 1 ? 0 : currStnNo);
		}
	}
}
#endif

// Things to do when in a loop, for example
void doCriticalTasks()
{
	// Get more data into circular buffer before we run out
	readHttpStream();
}