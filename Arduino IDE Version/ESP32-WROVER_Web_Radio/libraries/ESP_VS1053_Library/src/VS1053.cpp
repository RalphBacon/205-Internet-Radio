/**
 * This is a driver library for VS1053 MP3 Codec Breakout
 * (Ogg Vorbis / MP3 / AAC / WMA / FLAC / MIDI Audio Codec Chip).
 * Adapted for Espressif ESP8266 and ESP32 boards.
 *
 * version 1.0.1
 *
 * Licensed under GNU GPLv3 <http://gplv3.fsf.org/>
 * Copyright © 2017
 *
 * @authors baldram, edzelf, MagicCube, maniacbug
 *
 * Development log:
 *  - 2011: initial VS1053 Arduino library
 *          originally written by J. Coliz (github: @maniacbug),
 *  - 2016: refactored and integrated into Esp-radio sketch
 *          by Ed Smallenburg (github: @edzelf)
 *  - 2017: refactored to use as PlatformIO library
 *          by Marcin Szalomski (github: @baldram | twitter: @baldram)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License or later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ArduinoLog.h>
#include <VS1053.h>
#include <string>

VS1053::VS1053(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin)
        : cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin) {
}

uint16_t VS1053::read_register(uint8_t _reg) const {
    uint16_t result;

    control_mode_on();
    SPI.write(3);    // Read operation
    SPI.write(_reg); // Register to write (0..0xF)
    // Note: transfer16 does not seem to work
    result = (SPI.transfer(0xFF) << 8) | // Read 16 bits data
             (SPI.transfer(0xFF));
    await_data_request(); // Wait for DREQ to be HIGH again
    control_mode_off();
    return result;
}

void VS1053::write_register(uint8_t _reg, uint16_t _value) const {
    control_mode_on();
    SPI.write(2);        // Write operation
    SPI.write(_reg);     // Register to write (0..0xF)
    SPI.write16(_value); // Send 16 bits data
    await_data_request();
    control_mode_off();
}

void VS1053::sdi_send_buffer(uint8_t *data, size_t len) {
    size_t chunk_length; // Length of chunk 32 byte or shorter

    data_mode_on();
    while (len) // More to do?
    {
        await_data_request(); // Wait for space available
        chunk_length = len;
        if (len > vs1053_chunk_size) {
            chunk_length = vs1053_chunk_size;
        }
        len -= chunk_length;
        SPI.writeBytes(data, chunk_length);
        data += chunk_length;
    }
    data_mode_off();
}

void VS1053::sdi_send_fillers(size_t len) {
    size_t chunk_length; // Length of chunk 32 byte or shorter

    data_mode_on();
    while (len) // More to do?
    {
        await_data_request(); // Wait for space available
        chunk_length = len;
        if (len > vs1053_chunk_size) {
            chunk_length = vs1053_chunk_size;
        }
        len -= chunk_length;
        while (chunk_length--) {
            SPI.write(endFillByte);
        }
    }
    data_mode_off();
}

void VS1053::wram_write(uint16_t address, uint16_t data) {
    write_register(SCI_WRAMADDR, address);
    write_register(SCI_WRAM, data);
}

uint16_t VS1053::wram_read(uint16_t address) {
    write_register(SCI_WRAMADDR, address); // Start reading from WRAM
    return read_register(SCI_WRAM);        // Read back result
}

bool VS1053::testComm(const char *header) {
    // Test the communication with the VS1053 module.  The result wille be returned.
    // If DREQ is low, there is problably no VS1053 connected.  Pull the line HIGH
    // in order to prevent an endless loop waiting for this signal.  The rest of the
    // software will still work, but readbacks from VS1053 will fail.
    int i; // Loop control
    uint16_t r1, r2, cnt = 0;
    uint16_t delta = 300; // 3 for fast SPI

    if (!digitalRead(dreq_pin)) {
        Log.error("VS1053 not properly installed!" CR);
        // Allow testing without the VS1053 module
        pinMode(dreq_pin, INPUT_PULLUP); // DREQ is now input with pull-up
        return false;                    // Return bad result
    }
    // Further TESTING.  Check if SCI bus can write and read without errors.
    // We will use the volume setting for this.
    // Will give warnings on serial output if DEBUG is active.
    // A maximum of 20 errors will be reported.
    if (strstr(header, "Fast")) {
        delta = 3; // Fast SPI, more loops
    }

    std::string headerCr = std::string(header) + CR;
    Log.notice(headerCr.c_str());  // Show a header

    for (i = 0; (i < 0xFFFF) && (cnt < 20); i += delta) {
        write_register(SCI_VOL, i);         // Write data to SCI_VOL
        r1 = read_register(SCI_VOL);        // Read back for the first time
        r2 = read_register(SCI_VOL);        // Read back a second time
        if (r1 != r2 || i != r1 || i != r2) // Check for 2 equal reads
        {
            Log.error("VS1053 error retry SB:%04X R1:%04X R2:%04X" CR, i, r1, r2);
            cnt++;
            delay(10);
        }
        yield(); // Allow ESP firmware to do some bookkeeping
    }
    return (cnt == 0); // Return the result
}

bool VS1053::begin() {
    bool result = false;
    pinMode(dreq_pin, INPUT); // DREQ is an input
    pinMode(cs_pin, OUTPUT);  // The SCI and SDI signals
    pinMode(dcs_pin, OUTPUT);
    digitalWrite(dcs_pin, HIGH); // Start HIGH for SCI en SDI
    digitalWrite(cs_pin, HIGH);
    delay(100);
    Log.notice("Reset VS1053..." CR);
    digitalWrite(dcs_pin, LOW); // Low & Low will bring reset pin low
    digitalWrite(cs_pin, LOW);
    delay(500);
    Log.notice("End reset VS1053..." CR);
    digitalWrite(dcs_pin, HIGH); // Back to normal again
    digitalWrite(cs_pin, HIGH);
    delay(500);
    // Init SPI in slow mode ( 0.2 MHz )
    VS1053_SPI = SPISettings(200000, MSBFIRST, SPI_MODE0);
    // printDetails ( "Right after reset/startup" ) ;
    delay(20);
    // printDetails ( "20 msec after reset" ) ;
    result = testComm("Slow SPI,Testing VS1053 read/write registers...");

    //softReset();

    // Switch on the analog parts
    write_register(SCI_AUDATA, 44101); // 44.1kHz stereo
    // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
    write_register(SCI_CLOCKF, 6 << 12); // Normal clock settings multiplyer 3.0 = 12.2 MHz
    // SPI Clock to 4 MHz. Now you can set high speed SPI clock.
    VS1053_SPI = SPISettings(4000000, MSBFIRST, SPI_MODE0);
    write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_LINE1));
    result = testComm("Fast SPI, Testing VS1053 read/write registers again...");
    delay(10);
    await_data_request();
    endFillByte = wram_read(0x1E06) & 0xFF;
    Log.notice("endFillByte is %X" CR, endFillByte);
    // printDetails ( "After last clocksetting" ) ;
    delay(100);
    return result;
}

void VS1053::setVolume(uint8_t vol) {
    // Set volume.  Both left and right.
    // Input value is 0..100.  100 is the loudest.
    uint16_t value; // Value to send to SCI_VOL

    if (vol != curvol) {
        curvol = vol;                         // Save for later use
        value = map(vol, 0, 100, 0xFF, 0x00); // 0..100% to one channel
        value = (value << 8) | value;
        write_register(SCI_VOL, value); // Volume left and right
    }
}

void VS1053::setTone(uint16_t rtone) // Set bass/treble (4 nibbles)
{
	// RSB Could not get this to work, override
    // Set tone characteristics.  See documentation for the 4 nibbles.
    // uint16_t value = 0; // Value to send to SCI_BASS
    // int i;              // Loop control

    // for (i = 0; i < 8; i++) {
    //     value = (value << 4) | rtone[i]; // Shift next nibble in
    // }
    write_register(SCI_BASS, rtone);
}

uint8_t VS1053::getVolume() // Get the currenet volume setting.
{
    return curvol;
}

void VS1053::startSong() {
    sdi_send_fillers(10);
}

void VS1053::playChunk(uint8_t *data, size_t len) {
    sdi_send_buffer(data, len);
}

void VS1053::stopSong() {
    uint16_t modereg; // Read from mode register
    int i;            // Loop control

    sdi_send_fillers(2052);
    delay(10);
    write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_CANCEL));
    for (i = 0; i < 200; i++) {
        sdi_send_fillers(32);
        modereg = read_register(SCI_MODE); // Read status
        if ((modereg & _BV(SM_CANCEL)) == 0) {
            sdi_send_fillers(2052);
            Log.notice("Song stopped correctly after %d msec" CR, i * 10);
            return;
        }
        delay(10);
    }
    printDetails("Song stopped incorrectly!");
}

void VS1053::softReset() {
    write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET));
    delay(10);
    await_data_request();
}

void VS1053::printDetails(const char *header) {
    uint16_t regbuf[16];
    uint8_t i;

    std::string headerCr = std::string(header) + CR;
    Log.notice(headerCr.c_str());

    Log.notice("REG   Contents" CR);
    Log.notice("---   -----" CR);
    for (i = 0; i <= SCI_num_registers; i++) {
        regbuf[i] = read_register(i);
    }
    for (i = 0; i <= SCI_num_registers; i++) {
        delay(5);
        Log.notice("%3X - %5X" CR, i, regbuf[i]);
    }
}

/**
 * An optional switch.
 * Most VS1053 modules will start up in MIDI mode. The result is that there is no audio when playing MP3.
 * You can modify the board, but there is a more elegant way without soldering.
 * No side effects for boards which do not need this switch. It means you can call it just in case.
 *
 * Read more here: http://www.bajdi.com/lcsoft-vs1053-mp3-module/#comment-33773
 */
void VS1053::switchToMp3Mode(void)
{
    wram_write(0xC017, 3); // GPIO DDR = 3
    wram_write(0xC019, 0); // GPIO ODATA = 0
    delay(100);
    softReset();
}