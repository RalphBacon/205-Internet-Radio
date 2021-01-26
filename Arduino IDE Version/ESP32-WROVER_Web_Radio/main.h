// Ensure this header file is only included once
#ifndef _ESP32RADIO_
#define _ESP32RADIO_

// Set the debugging level. Done automatically with PlatformIO but not for Arduino.
#ifndef CORE_DEBUG_LEVEL
// ; ARDUHAL_LOG_LEVEL +
// ;	_NONE       (0) // Stumm. Nada. On your own.
// ;	_ERROR      (1) // Usually fatal errors
// ;	_WARN       (2) // Only when things go wrong
// ;	_INFO       (3) // Useful just to see it working
// ;	_DEBUG      (4) // Debugging programming
// ;	_VERBOSE    (5) // Every message
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_DEBUG
#endif

/*
	Includes go here
*/
// Utility to write to (pseudo) EEPROM
#include <Preferences.h>

// Standard ESP32 WiFi (not secure https)
#include <WiFi.h>

// MP3+ decoder
#include <VS1053.h>

// Standard input/output streams, required for <locale> but this might get removed
#include <iostream>

// SPIFFS (in-memory SPI File System) replacement
#include <LITTLEFS.h>

// Display / TFT
#include <User_Setup.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"

// Your preferred TFT / LED screen definition here
// As I'm using the TFT_eSPI from Bodmer they are all in User_Setup.h

// Circular buffer - ESP32 built in (hacked by me to use PSRAM)
#include <cbuf.h>

// EEPROM writing routines (eg: remembers previous radio stn)
Preferences preferences;

// Station index/number
unsigned int currStnNo, prevStnNo;
signed int nextStnNo;

// Secret WiFi stuff from LITTLEFS (SPIFFS+)
std::string ssid;
std::string wifiPassword;
bool wiFiDisconnected = true;

// All radio connections are assumed insecure http:// not https://

/* 
    Example URL: [port optional, 80 assumed]
        stream.antenne1.de[:80]/a1stg/livestream1.aac Antenne1 (Stuttgart)
*/

// Pushbutton connected to this pin to change station
int stnChangePin = 13;
int tftTouchedPin = 15;
uint prevTFTBright;

// Can we use the above button (not in the middle of changing stations)?
bool canChangeStn = true;

// Current state of WiFi (connected, idling)
int status = WL_IDLE_STATUS;

// Do we want to connect with track/artist info (metadata)
bool METADATA = true;

// Whether to request ICY data or not. Overridden if set to 0 in radio stations.
#define ICYDATAPIN 36 // Input only pin, requires pull-up 10K resistor

// The number of bytes between metadata (title track)
uint16_t metaDataInterval = 0; //bytes
uint16_t bytesUntilmetaData = 0;
int bitRate = 0;
bool redirected = false;
bool volumeMax = false;

// Dedicated 32-byte buffer for VS1053 aligned on 4-byte boundary for efficiency
uint8_t mp3buff[32] __attribute__((aligned(4)));

// Circular "Read Buffer" to stop stuttering on some stations
#ifdef BOARD_HAS_PSRAM
#define CIRCULARBUFFERSIZE 150000 // Divide by 32 to see how many 2mS samples this can store
#else
#define CIRCULARBUFFERSIZE 10000
#endif
cbuf circBuffer(10);
#define streamingCharsMax 32

// Internet stream buffer that we copy in chunks to the ring buffer
char readBuffer[100] __attribute__((aligned(4)));

// Wiring of VS1053 board (SPI connected in a standard way) on ESP32 only
#define VS1053_CS 32
#define VS1053_DCS 33
#define VS1053_DREQ 35

// Volume settings user=as set by user, target=current program required volume (eg fading out)
uint8_t targetVolume = 95; // treble/bass works better if NOT 100 here
uint8_t userVolume = 0;	   // stored in EEPROM

// Delay for music fade in after station change (glitchy sound)
#define MIN_DELAY_AFTER_STATION_CHANGE 1200
unsigned long timeAtStationChange;

// WiFi specific defines
#define WIFITIMEOUTSECONDS 20

/* 
	Forward declarations of functions TODO: clean up & describe FIXME:
*/

// WiFi helper functions
void connectToWifi();
std::string getWiFiPassword();
std::string getSSID();
const char *wl_status_to_string(wl_status_t status);
// End

// TFT Helper routines
void initDisplay();
void setupDisplayModule(bool initialiseHardware = false);
std::string toTitle(std::string s, const std::locale &loc = std::locale());
void drawStnChangeButton();
void drawStnChangeBitmap(bool pressed = false);
void drawNextButton();
void drawSpkrButton(bool invert);
void drawSpkrBitmap();
void drawDummyPlusMinusButtons();
void drawPlusButton(bool invert);
void drawMinusButton(bool invert);
void drawPercentageLine(int lineType);
void drawBulbButton();
void drawBulbBitmap(bool active = false);
void drawBufferLevel(size_t bufferLevel, bool override = false);
bool getStnChangeButtonPress(uint16_t x, uint16_t y);
void getPlusButtonPress();
void getMinusButtonPress();
void displayStationName(char *stationName);
void displayTrackArtist(std::string);
// End

// Loop functions
void readHttpStream();
void populateRingBuffer();
bool stationConnect(int station_no, bool clearBuffer = true);
std::string readLITTLEFSInfo(char *itemRequired);
bool changeStation(int8_t stnNumber);
bool _GLIBCXX_ALWAYS_INLINE readMetaData();
void getRedirectedStationInfo(String header, int currStationNo);
void checkForStationChange();
void fadeOutMusic();
void fadeInMusic(int playerVol);
void checkForNextButton();
bool delaySinceStationChange();

// Task Play Music Helper functions
void taskSetup();
void checkBufferForPlaying();
bool playMusicFromRingBuffer();

// Which page are we on? Home page = normal use, stnselect is list of stations
enum screenPage
{
	HOME,
	STNSELECT
	// This will be expanded as I develop a menu structure
};
screenPage currDisplayScreen = HOME;

// Button status to determine what has control over +/- buttons
enum btnStatus
{
	BTN_INACTIVE,
	BTN_ACTIVE
};
btnStatus bulbStatus = BTN_INACTIVE;
btnStatus spkrStatus = BTN_INACTIVE;
bool isMutedState = false;

// The current Artist/Title (plus much more) info
std::string currstreamArtistTitle = "";

// global flag to indicate whether we successfully connected to a radio station
bool connectedToStation = false;

/*
	Instantiate objects here so they can be referenced by all 'helper' code
	TODO: Move to the correct "helper" file
*/

// MP3 decoder
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

// Instantiate screen (object) using hardware SPI. Defaults to 320H x 240W
TFT_eSPI tft = TFT_eSPI();

// Start the WiFi client here
WiFiClient client;

#endif