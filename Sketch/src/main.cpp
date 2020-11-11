#include <Arduino.h>
#include "main.h"

// ==================================================================================
// setup	setup	setup	setup	setup	setup	setup	setup	setup
// ==================================================================================
void setup()
{
	// Debug monitor TODO disable if not in debugging mode
	Serial.begin(115200);

	// Start the display so we can show connection/hardware errors on it
	initDisplay();

	// Initialise LITTLEFS system
	if (!LITTLEFS.begin(false))
	{
		Serial.println("LITTLEFS Mount Failed.");
		while (1)
			;
	}
	else
	{
		Serial.println("LITTLEFS Mount SUCCESSFUL.");
		ssid = getSSID();
		wifiPassword = getWiFiPassword();
		connectToWifi();
		if (wiFiDisconnected)
		{
			Serial.printf("Unable to connect to WiFi network: %s\n", ssid.c_str());
			while (1)
				;
		}
	}

	// Station change pin TODO: move to interrupt pin, no polling required, simplifies coding
	pinMode(stnChangePin, INPUT_PULLUP);
	pinMode(tftTouchedPin, INPUT);

	// initialize SPI bus;
	SPI.begin();

	// VS1053 MP3 decoder
	player.begin();

	// Wait for the player to be ready to accept data
	Serial.println("Waiting for VS1053 initialisation to complete.");
	while (!player.data_request())
	{
		delay(1);
	}

	// You MIGHT have to set the VS1053 to MP3 mode. No harm done if not required!
	player.switchToMp3Mode();

	// Set the volume here to MAX (100)
	// This now moved to station connection function to avoid bad sound when connecting
	player.setVolume(100);

	// Set the equivalent of "Loudness" (increased bass & treble)
	// Bits 15:12	treble control in 1.5dB steps (-8 to +7, 0 = off)
	// Bits 11:8	lower limit frequency in 1kHz steps (1kHz to 15kHz)
	// Bits 7:4		Bass Enhancement in 1dB steps (0 - 15, 0 = off)
	// Bits 3:0		Lower limit frequency in 10Hz steps (2 - 15)
	char trebleCutOff = 3; // 3kHz cutoff
	char trebleBoost = 3 << 4 | trebleCutOff;
	char bassCutOff = 10; // times 10 = 100Hz cutoff
	char bassBoost = 3 << 4 | bassCutOff;

	uint16_t SCI_BASS = trebleBoost << 8 | bassBoost;
	// equivalent of player.setTone(0b0111001110101111);
	player.setTone(SCI_BASS);

	// Some sort of startup message
	// TODO change to MP3 file on LITTLEFS
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
		Serial.println("Unable to open greetings file");
	}

	// Set the volume here to MIN (0)
	player.setVolume(0);

	// Get the station number that was previously playing
	preferences.begin("WebRadio", false);
	currStnNo = preferences.getUInt("currStnNo", 0);
	if (currStnNo > stationCnt - 1)
	{
		currStnNo = 0;
	}
	prevStnNo = currStnNo;

	// Connect to that station
	Serial.printf("Current station number: %u\n", currStnNo);
	bool isConnected = false;
	do
	{
		isConnected = station_connect(currStnNo);
	} while (!isConnected);

	//Initialise the circular buffer
	circBuffer.flush();
}

// ==================================================================================
// loop     loop     loop     loop    loop     loop     loop     loop    loop    loop
// ==================================================================================
void loop()
{
	// Did we run out of data to send the VS1053?
	static bool dataPanic = false;

	// Data to read from mainBuffer?
	if (client.available())
	{
		int bytesReadFromStream = 0;

		// Room in the ring buffer for (up to) 100 bytes?
		if (circBuffer.room() > 99)
		{
			// Read either the maximum available (max 100) or the number of bytes to the next meata data interval
			bytesReadFromStream = client.read((uint8_t *)readBuffer, min(100, (int)bytesUntilmetaData));

			// If we get -1 here it means nothing could be read from the stream
			if (bytesReadFromStream > 0)
			{
				// Add them to the circular buffer
				circBuffer.write(readBuffer, bytesReadFromStream);

				// Some radio stations (eg BBC Radio 4!!!) limit the data to 92 bytes. Why?
				if (bytesReadFromStream < 92 && bytesReadFromStream != bytesUntilmetaData)
				{
					Serial.printf("Only wrote %db to circ buff\n", bytesReadFromStream);
				}
			}
		}
		else
		{
			// There will be thousands of this message. Only for debugging.
			//Serial.println("Circ buff full.");
		}

		// Subtract bytes actually read from incoming http data stream from the bytesUntilmetaData
		bytesUntilmetaData -= bytesReadFromStream;

		// If it's reached zero, check to see if there is information to read
		if (bytesUntilmetaData == 0)
		{
			readMetaData();
			bytesUntilmetaData = metaDataInterval; // reset byte count for next bit of metadata
		}
	}

	// If the buffer is nearly full after a station change allow the buffer to be played
	if (bufferChangedStation)
	{
		// Now read (up to) 32 bytes of audio data and play it
		if (circBuffer.available())
		{
			// Does the VS1053 want any more data (yet)?
			if (player.data_request())
			{
				{
					// Read the data from the circuluar (ring) buffer
					int bytesRead = circBuffer.read((char *)mp3buff, 32);

					// If we didn't read the full 32 bytes, that's a worry
					if (bytesRead != 32)
					{
						Serial.printf("Only read %d bytes from  circular buffer\n", bytesRead);
					}

					// Actually send the data to the VS1053
					player.playChunk(mp3buff, bytesRead);
				}
			}
		}
		else
		{
			if (!dataPanic)
			{
				//Serial.println("PANIC No audio data to read from circular buffer");
				// TODO: we might need to auto reconnect here?
				dataPanic = true;
			}
			else
			{
				dataPanic = false;
			}
		}
	}
	else
	{
		if (circBuffer.available() > CIRCULARBUFFERSIZE / 2)
		{
			// Reset the flag, allow data to be played, won't get reset until station change
			bufferChangedStation = true;

			// Volume is set to zero before we have a valid connection. If we haven't
			// yet turned up the volume, now is the time to do that.
			if (!volumeMax)
			{
				player.setVolume(VOLUME);
				volumeMax = true;
			}
		}
		else
		{
			//Serial.printf("Buffering: %d\n",circBuffer.available());
		}
	}

	// So how many bytes have we got in the buffer (should hover around 9,900)
	// TODO: see if we can send this to the screen, too disruptive here
	//Serial.printf("Buff:%d\n",circBuffer.available());
	drawBufferLevel(circBuffer.available());

	// Has CHANGE STATION button been pressed?
	if (!digitalRead(stnChangePin) && changeStnButton == true)
	{
		changeStation();
	}
	else
	{
		if (!digitalRead(tftTouchedPin) && changeStnButton == true)
		{
			if (getNextButtonPress())
			{
				changeStation();
			}
		}
	}
}

// Connect to the station list number
bool station_connect(int stationNo)
{
	Serial.println("--------------------------------------");
	Serial.println("        Connecting to station");
	Serial.println("--------------------------------------");

	// Turn down the volume (will be reset once we've got some data)
	player.setVolume(0);
	volumeMax = false;

	// Set the metadataInterval value to zero we can detect that we found a valid one
	metaDataInterval = 0;

	// Clear down any screen info
	displayStationName(radioStation[stationNo].friendlyName);
	displayTrackArtist((char *)"");
	drawBufferLevel(0);

	// We try a few times to connect to the station
	bool connected = false;
	int connectAttempt = 0;
	while (!connected && connectAttempt < 5)
	{
		if (redirected)
		{
			Serial.printf("REDIRECTED URL DETECTED FOR STATION %d\n", stationNo);
		}

		connectAttempt++;
		Serial.printf("Host:%s Port:%d\n", radioStation[stationNo].host, radioStation[stationNo].port);
		if (client.connect(radioStation[stationNo].host, radioStation[stationNo].port))
		{
			connected = true;
		}
		else
		{
			delay(250);
		}
	}

	// If we could not connect (eg bad URL) just exit
	if (!connected)
	{
		Serial.printf("Could not connect to %s", radioStation[stationNo].host);
		return false;
	}
	else
	{
		Serial.printf("Connected to %s (%s%s)\n",
					  radioStation[stationNo].host, radioStation[stationNo].friendlyName,
					  redirected ? " - redirected" : "");
	}

	// Get the data stream plus any metadata (eg station name, track info between songs / ads)
	// TODO: Allow retries here (BBC Radio 4 very finicky before streaming).
	// We might also get a redirection URL given back.
	Serial.printf("Getting data from %s\n", radioStation[stationNo].path);
	client.print(
		String("GET ") + radioStation[stationNo].path + " HTTP/1.1\r\n" +
		"Host: " + radioStation[stationNo].host + "\r\n" +
		"Icy-MetaData:1\r\n" +
		"Connection: close\r\n\r\n");

	// Give the client a chance to connect
	while (client.available() == 0)
	{
		Serial.println("No data yet from connection.");
		delay(500);
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

		if (responseLine.indexOf("Status: 200 OK") > 0)
		{
			// TODO: we really should check we have a response of 200 before anything else
			Serial.println("200 - OK response");
			continue;
		}

		// If we have an EMPTY header (or just a CR) that means we had two linefeeds
		if (responseLine[0] == (uint8_t)13 || responseLine == "")
		{
			break;
		}

		// If the header is not empty process it
		Serial.print("HEADER:");
		Serial.println(responseLine);
		Serial.println("");

		// Critical value for this whole sketch to work: bytes between "frames"
		// Sometimes you can't get this first time round so we just reconnect
		if (responseLine.startsWith("icy-metaint"))
		{
			metaDataInterval = responseLine.substring(12).toInt();
			Serial.printf("NEW Metadata Interval:%d\n", metaDataInterval);
			continue;
		}

		// The bit rate of the transmission (FYI) eye candy
		if (responseLine.startsWith("icy-br:"))
		{
			bitRate = responseLine.substring(7).toInt();
			Serial.printf("Bit rate:%d\n", bitRate);
			continue;
		}

		// TODO: Remove this testing override for station 4 (always redirects!)
		// The URL we used has been redirected
		if (!redirected && stationNo == 4)
		{
			responseLine = "location: http://stream.antenne1.de:80/a1stg/livestream1.aac";
		}

		if (responseLine.startsWith("location: http://"))
		{
			getRedirectedStationInfo(responseLine, stationNo);
			redirected = true;
			return false;
		}
	}

	// Update the count of bytes until the next metadata interval (used in loop) and exit
	bytesUntilmetaData = metaDataInterval;

	// Display the name of the radio station even if we didn't get the correct metadata
	displayStationName(radioStation[stationNo].friendlyName);

	// If we didn't find a metaDataInterval value in the headers, abort this connection
	if (bytesUntilmetaData == 0)
	{
		Serial.println("NO METADATA INTERVAL DETECTED - RECONNECTING");
		return false;
	}

	// Clear down the streaming buffer and reset the player
	circBuffer.flush();
	player.softReset();

	// Flag to indicate we need to buffer before allowing player to stream audio
	bufferChangedStation = false;

	// All done here
	return true;
}

// Get the WiFi SSID
std::string getSSID()
{
	std::string currSSID = readLITTLEFSInfo((char *)"SSID");
	if (currSSID == "")
	{
		return "Benny";
	}
	else
	{
		return currSSID;
	}
}

// Get the WiFi Password
std::string getWiFiPassword()
{
	std::string currWiFiPassword = readLITTLEFSInfo((char *)"WiFiPassword");
	if (currWiFiPassword == "")
	{
		return "BestCatEver";
	}
	else
	{
		return currWiFiPassword;
	}
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

	Serial.printf("Looking for key '%s'.\n", itemRequired);

	// Get a handle to the file
	File configFile = LITTLEFS.open("/WiFiSecrets.txt", FILE_READ);
	if (!configFile)
	{
		// TODO: Display error on screen
		Serial.println("Unable to open file /WiFiSecrets.txt");
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
			//Serial.print("Throwing away preMarker:");
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

		//Serial.printf("Found: '%s'.\n", receivedChars);
		if (strcmp(receivedChars, itemRequired) == 0)
		{
			//Serial.println("Found matching key - next string will be returned");
			foundKey = true;
		}
	}

	// Terminate file
	configFile.close();

	// Did we find anything
	Serial.printf("LITTLEFS parameter '%s'\n", itemRequired);
	if (charCnt == 0)
	{
		Serial.println("' not found.");
		return "";
	}
	else
	{
		//Serial.printf("': '%s'\n", receivedChars);
	}

	return receivedChars;
}

// Connect to WiFi
void connectToWifi()
{
	Serial.printf("Connecting to SSID: %s\n", ssid.c_str());

	WiFi.mode(WIFI_STA);
	WiFi.persistent(false);

	// Don't allow WiFi sleep
	WiFi.setSleep(false);

	//Coonect to the required WiFi
	WiFi.begin(ssid.c_str(), wifiPassword.c_str());

	// Try to connect a few times before timing out
	int timeout = WIFITIMEOUTSECONDS * 4;
	while (WiFi.status() != WL_CONNECTED && (timeout-- > 0))
	{
		delay(500);
		WiFi.reconnect();
		delay(500);
		Serial.print(".");
	}

	// Successful connection?
	wl_status_t wifiStatus = WiFi.status();
	if (wifiStatus != WL_CONNECTED)
	{
		Serial.println("\nFailed to connect, exiting.");
		Serial.printf("WiFi Status: %s\n", wl_status_to_string(wifiStatus));
		return;
	}

	Serial.printf("WiFi connected with (local) IP address of: %s\n", WiFi.localIP().toString().c_str());
	wiFiDisconnected = false;
}

// Convert the WiFi (error) response to a string we can understand
const char *wl_status_to_string(wl_status_t status)
{
	switch (status)
	{
	case WL_NO_SHIELD:
		return "WL_NO_SHIELD";
	case WL_IDLE_STATUS:
		return "WL_IDLE_STATUS";
	case WL_NO_SSID_AVAIL:
		return "WL_NO_SSID_AVAIL";
	case WL_SCAN_COMPLETED:
		return "WL_SCAN_COMPLETED";
	case WL_CONNECTED:
		return "WL_CONNECTED";
	case WL_CONNECT_FAILED:
		return "WL_CONNECT_FAILED";
	case WL_CONNECTION_LOST:
		return "WL_CONNECTION_LOST";
	case WL_DISCONNECTED:
		return "WL_DISCONNECTED";
	default:
		return "UNKNOWN";
	}
}

void changeStation()
{
	Serial.println("--------------------------------------");
	Serial.println("            Change Station");
	Serial.println("--------------------------------------");

	// Make button inactive
	changeStnButton = false;

	// Reset any redirection flag
	redirected = false;

	// Get the next station (in the list)
	nextStnNo = currStnNo + 1;
	if (nextStnNo > stationCnt - 1)
	{
		nextStnNo = 0;
	}

	// Whether connected to this station or not, update the variable otherwise
	// we would 'stick' at old station
	currStnNo = nextStnNo;

	if (prevStnNo != nextStnNo)
	{
		prevStnNo = nextStnNo;
		player.softReset();

		// Now actually connect to the new URL for the station
		bool isConnected = false;
		do
		{
			isConnected = station_connect(nextStnNo);
		} while (!isConnected);

		// Next line might not be required for VS051
		player.switchToMp3Mode();

		// Store (new) current station in EEPROM
		preferences.putUInt("currStnNo", nextStnNo);
		Serial.printf("Current station now set to: %u\n", nextStnNo);

		// Button active again
		changeStnButton = true;
	}
}

// Display routines (hardware/protocol dependent)
void initDisplay()
{
	// TODO: display something on the LCD etc
	setupDisplayModule();
}

std::string readMetaData()
{
	// The first byte is going to be the length of the metadata
	int metaDataLength = client.read();

	// Usually there is none as track/artist info is only updated when it changes
	// It may also return the station URL (not necessarily the same as we are using).
	// Example:
	//  'StreamTitle='Love Is The Drug - Roxy Music';StreamUrl='https://listenapi.planetradio.co.uk/api9/eventdata/62247302';'
	if (metaDataLength == 0)
	{
		// Warning sends out about 4 lines per second!
		//Serial.println("No metadata to read.");
		return "";
	}

	// The actual length is 16 times bigger to allow from 16 to up to 4080 bytes (255 * 16) of metadata
	metaDataLength = (metaDataLength * 16);
	Serial.printf("Metadata block size: %d\n", metaDataLength);

	for (auto cnt = 0; cnt < metaDataLength; cnt++)
	{
		metaData[cnt] = client.read();
		// TODO: if we have unprintable chars here it cannot be stream title so abort
	}

	// Terminate the "string" with a null character
	metaData[metaDataLength] = '\0';
	Serial.printf("Found raw metaData:%s\n", metaData);

	// Extract track Title/Artist from this string
	char *startTrack = NULL;
	char *endTrack = NULL;
	std::string streamArtistTitle = "";

	startTrack = strstr(metaData, "StreamTitle");
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

		ptrdiff_t startIdx = startTrack - metaData;
		ptrdiff_t endIdx = endTrack - metaData;
		std::string streamInfo(metaData, startIdx, endIdx);
		streamArtistTitle = streamInfo;
		Serial.printf("%s\n", streamArtistTitle.c_str());
		displayTrackArtist(toTitle(streamArtistTitle));
	}

	// Return a 'titled' title, eg THIS IS A TITLE gets converted to This Is A Title
	return toTitle(streamArtistTitle);
}

// Our streaming station has been 'redirected' to another URL
// The header will look like: Location: http://<new host / path>[:<port>]
void getRedirectedStationInfo(String header, int currStationNo)
{
	Serial.println("--------------------------------------");
	Serial.println(" Extracting redirection information");
	Serial.println("--------------------------------------");

	// Placeholders for the new host/path
	String redirectedHost = "";
	String redirectedPath = "";

	// We'll assume the port is 80 unless we find one in the host name
	int redirectedPort = 80;

	// Skip the "redirected http://" bit at the front
	header = header.substring(17);
	Serial.printf("Redirecting to: %s\n", header.c_str());

	// Split the header into host and path constituents
	int pathDelimiter = header.indexOf("/");
	if (pathDelimiter > 0)
	{
		redirectedPath = header.substring(pathDelimiter);
		redirectedHost = header.substring(0, pathDelimiter);
	}
	// Look to split host into host and port number
	// Example: stream/myradio.de:8080
	int portDelimter = header.indexOf(":");
	if (portDelimter > 0)
	{
		redirectedPort = header.substring(portDelimter + 1).toInt();

		// Adjust the host name to exclude the port information
		redirectedHost = redirectedHost.substring(0, portDelimter);
	}

	// Just overwrite the current entry for this station (reverts on reboot)
	// TODO: consider writing all this to EEPROM / SPIFFS
	Serial.println("New address: " + redirectedHost + redirectedPath + ":" + redirectedPort);
	strncpy(radioStation[currStationNo].host, redirectedHost.c_str(), 64);
	strncpy(radioStation[currStationNo].path, redirectedPath.c_str(), 128);

	return;
}

// Put the TFT specific code here to prepare your screen
void setupDisplayModule()
{

	// Initialise
	tft.init();

	// Rotation as required
	tft.setRotation(1);

	// Write title of app on screen, using font 2 (x, y, font #)
	tft.fillScreen(TFT_BLACK);

	// Border + fill
	tft.fillRect(1, 1, 318, 43, TFT_RED);
	tft.drawRect(0, 0, 320, 45, TFT_YELLOW);

	// Application Title
	tft.setFreeFont(&FreeSansBold12pt7b);
	tft.setTextSize(1);
	tft.setCursor(32, 30);

	// Set text colour and background
	tft.setTextColor(TFT_YELLOW, TFT_RED);
	tft.println("- ESP32 WEB RADIO -");

	// My details
	tft.setTextFont(2);
	tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
	tft.setCursor(5, tft.height() - 20);
	tft.println("Ralph S Bacon - https://youtube.com/ralphbacon");

	// Draw button(s)
	drawNextButton();

	// All done
	Serial.println("TFT Initialised");
	return;
}

void displayStationName(char *stationName)
{
	// Set text colour and background
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);

	// Clear the remainder of the line from before (eg long title)
	tft.fillRect(0, 56, 320, 40, TFT_BLACK);

	// Write station name (as stored in the sketch)
	tft.setCursor(0, 85);
	tft.setFreeFont(&FreeSansOblique12pt7b);
	tft.setTextSize(1);
	tft.println(stationName);
}

void displayTrackArtist(std::string trackArtist)
{
	// Set text colour and background
	tft.setTextColor(TFT_GREEN, TFT_BLACK);

	// Clear the remainder of the line from before (eg long title)
	tft.fillRect(0, 100, 320, 50, TFT_BLACK);

	// Write artist / track info
	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setCursor(0, 120);
	tft.print(trackArtist.c_str());
}

// TODO: FIXME: BUG: The touch screen has Y coord as 0 bottom of screen
// Switch position and size
#define FRAME_X 120
#define FRAME_Y 160
#define FRAME_W 140
#define FRAME_H 40

// Red zone size
#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W / 2)
#define REDBUTTON_H FRAME_H

bool getNextButtonPress()
{
	// To store the touch coordinates
	uint16_t t_x = 0, t_y = 0;

	// Get current touch state and coordinates
	boolean pressed = tft.getTouch(&t_x, &t_y, 200);

	if (pressed)
	{
		Serial.printf("Touch at x:%d y:%d\n", t_x, t_y);
		if (t_x > REDBUTTON_X && (t_x < (REDBUTTON_X + REDBUTTON_W)))
		{
			//if ((t_y > 240 - (REDBUTTON_Y + REDBUTTON_H)) && (t_y >= (REDBUTTON_Y + REDBUTTON_H)))
			if (t_y > 30 && t_y < 70)
			{
				Serial.println("Hit!");
				return true;
			}
		}
	}

	return false;
}

// Draw proof-of-concept NEXT button TODO: expose coordinates
void drawNextButton()
{
	tft.fillRoundRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, 5, TFT_RED);
	tft.drawRoundRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, 5, TFT_BLACK);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(2);
	tft.setTextDatum(MC_DATUM);
	tft.drawString("NEXT", REDBUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
}

// Draw percentage of buffering (eg 10000 buffer that has 8000 bytes = 80%)
void drawBufferLevel(size_t bufferLevel)
{
	static unsigned long prevMillis = millis();
	static uint8_t prevBufferPerCent = 0;

	// Only do this infrequently and if the buffer % changes
	if (millis() - prevMillis > 500)
	{
		// Capture current values for next time
		prevMillis = millis();

		// Calculate the percentage (ESP32 has FP processor so shoud be efficient)
		float bufLevel = (float)bufferLevel;
		float arraySize = (float)(CIRCULARBUFFERSIZE);
		int bufferPerCent = (bufLevel / arraySize) * 100.0;

		// Only uipdate the screen on real change (avoids flicker)
		if (bufferPerCent != prevBufferPerCent)
		{
			// Print at specific rectangular place
			// TODO: These should not be magic numbers
			tft.fillRoundRect(245, 170, 60, 30, 5, TFT_DARKGREEN);

			tft.setTextColor(TFT_YELLOW, TFT_DARKGREEN);
			tft.setFreeFont(&FreeSans9pt7b);
			tft.setTextSize(1);
			tft.setCursor(255, 191);
			tft.printf("%d%%\n", bufferPerCent);
		}
	}
}

// Some track titles are ALL IN UPPER CASE, so ugly, so let's convert them
// Stackoverflow: https://stackoverflow.com/a/64128715
std::string toTitle(std::string s, const std::locale &loc)
{
	// Is the current character a word delimiter (a space)?
	bool last = true;

	// Process each character in the supplied string
	for (char &c : s)
	{
		// If the previous character was a word delimiter (space)
		// then upper case current word start, else make lower case
		c = last ? std::toupper(c, loc) : std::tolower(c, loc);

		// Is this character a space?
		last = std::isspace(c, loc);
	}
	return s;
}