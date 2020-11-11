#ifndef _ESP32RADIO_
#define _ESP32RADIO_

// Includes go here
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wifi.h>
#include <VS1053.h>
#include <iostream>
#include <LITTLEFS.h>

// Display / TFT
#include <User_Setup.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"

// Circular buffer
#include <cbuf.h>

// EEPROM writing routines (eg: remembers previous radio stn)
Preferences preferences;
unsigned int currStnNo, prevStnNo, nextStnNo;

// Your preferred TFT / LED screen definition here
// As I'm using the TFT_eSPI from Bodmer they are all in User_Setup.h

// Secret stuff from LITTLEFS (SPIFFS+)
std::string ssid;
std::string wifiPassword;
bool wiFiDisconnected = true;
WiFiClient client;

// All connections are assumed insecure http:// not https://
const int8_t stationCnt = 9; // start counting from 1!

/* 
    Example URL: [port optional, 80 assumed]
        stream.antenne1.de[:80]/a1stg/livestream1.aac Antenne1 (Stuttgart)
*/

struct radioStationLayout
{
    char host[64];
    char path[128];
    int port;
    char friendlyName[64];
};

struct radioStationLayout radioStation[stationCnt] = {

    //0
    "stream.antenne1.de",
    "/a1stg/livestream1.aac",
    80,
    "Antenne1.de",

    //1
    "bbcmedia.ic.llnwd.net",
    "/stream/bbcmedia_radio4fm_mf_q", // also mf_p works
    80,
    "BBC Radio 4",

    //2
    "stream.antenne1.de",
    "/a1stg/livestream2.mp3",
    80,
    "Antenne1 128k",

    //3
    "listen.181fm.com",
    "/181-beatles_128k.mp3",
    80,
    "Beatles 128k",

    //4
    "stream-mz.planetradio.co.uk",
    "/magicmellow.mp3",
    80,
    "Mellow Magic (Redirected)",

    //5
    "edge-bauermz-03-gos2.sharp-stream.com",
    "/net2national.mp3",
    80,
    "Greatest Hits 112k (National)",

    //6
    "airspectrum.cdnstream1.com",
    "/1302_192",
    8024,
    "Mowtown Magic Oldies",

    //7
    "94.130.242.5",
    "/stream3",
    8024,
    "Party Vibe Radio: Top 40",

    "stream-mz.planetradio.co.uk",
    "/magicmellow.aac",
    80,
    "Mellow Magic (AAC)"

};

// Pushbutton connected to this pin to change station
int stnChangePin = 13;
int tftTouchedPin = 15;

// Can we use the above button (not in the middle of changing stations)?
bool changeStnButton = true;
bool bufferChangedStation = false;

// Current state of WiFi (connected, idling)
int status = WL_IDLE_STATUS;

// The number of bytes between metadata (title track)
uint16_t metaDataInterval = 0; //bytes
uint16_t bytesUntilmetaData = 0;
char metaData[4080];
bool firstTimeDataChunk = true;
int bitRate = 0;
bool redirected = false;
bool volumeMax = false;

// Dedicated 32-byte buffer for VS1053 aligned on 4-byte boundary for efficiency
uint8_t mp3buff[32] __attribute__((aligned(4)));

// Circular "Read Buffer" to stop stuttering on some stations
#define CIRCULARBUFFERSIZE 100000 // Divide by 32 to see how many 2mS samples this can store
cbuf circBuffer(CIRCULARBUFFERSIZE);

// Internet stream buffer that we copy in chunks to the ring buffer
char readBuffer[100] __attribute__((aligned(4)));

// Wiring of VS1053 board (SPI connected in a standard way)
#define VS1053_CS 32   //32
#define VS1053_DCS 33  //33
#define VS1053_DREQ 35 //15
#define VOLUME 100     // volume level 0-100
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

// WiFi specific defines
#define WIFITIMEOUTSECONDS 20

// Forward declarations of functions TODO: clean up & describe FIXME:
bool station_connect(int station_no);
std::string readLITTLEFSInfo(char *itemRequired);
std::string getWiFiPassword();
std::string getSSID();
void connectToWifi();
const char *wl_status_to_string(wl_status_t status);
void initDisplay();
void changeStation();
std::string readMetaData();
void syncToMp3Frame();
void getRedirectedStationInfo(String header, int currStationNo);
void setupDisplayModule();
void displayStationName(char *stationName);
void displayTrackArtist(std::string);
bool getNextButtonPress();
void drawNextButton();
std::string toTitle(std::string s, const std::locale &loc = std::locale());
void drawBufferLevel(size_t bufferLevel);

// Instatiate screen (object) using hardware SPI. Defaults to 320H x 240W
TFT_eSPI tft = TFT_eSPI();
#endif