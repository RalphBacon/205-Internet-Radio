/**
  A simple example to use ESP_VS1053_Library (plays a test sound every 3s)
  https://github.com/baldram/ESP_VS1053_Library
  If you like this project, please add a star.

  Copyright (C) 2017 Marcin Szalomski (github.com/baldram)
  Licensed under GNU GPL v3

  The circuit (example wiring for ESP8266 based board like eg. LoLin NodeMCU V3):
  ---------------------
  | VS1053  | ESP8266 |
  ---------------------
  |   SCK   |   D5    |
  |   MISO  |   D6    |
  |   MOSI  |   D7    |
  |   XRST  |   RST   |
  |   CS    |   D1    |
  |   DCS   |   D0    |
  |   DREQ  |   D3    |
  |   5V    |   VU    |
  |   GND   |   G     |
  ---------------------

  Note: It's just an example, you may use a different pins definition.
  For ESP32 example, please follow the link:
    https://github.com/baldram/ESP_VS1053_Library/issues/1#issuecomment-313907348

  To run this example define the platformio.ini as below.

  [env:nodemcuv2]
  platform = espressif8266
  board = nodemcuv2
  framework = arduino

  lib_deps =
    ESP_VS1053_Library

*/

// This ESP_VS1053_Library
#include <VS1053.h>

// Please find helloMp3.h file here:
//   github.com/baldram/ESP_VS1053_Library/blob/master/examples/SimpleMp3Player/helloMp3.h
#include "helloMp3.h"

// Wiring of VS1053 board (SPI connected in a standard way)
#define VS1053_CS     D1
#define VS1053_DCS    D0
#define VS1053_DREQ   D3

#define VOLUME  100 // volume level 0-100

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

void setup () {
    // initialize SPI
    SPI.begin();

    // initialize a player
    player.begin();
    player.switchToMp3Mode(); // optional, some boards require this
    player.setVolume(VOLUME);
}

void loop() {
    // play mp3 flow each 3s
    player.playChunk(helloMp3, sizeof(helloMp3));
    delay(3000);
}
