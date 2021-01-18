// All of the ILI9341 TFT Touch Screen routines are included here
#include "Arduino.h"
#include "main.h"

// Bitmap display helper (Bodmer's)
#include "bitmapHelper.h"

// IMPORTANT: you MUST run the touch calibration sketch to store the values
// for future runs of the sketch (ie run once, in the orientation you expect
// to use the screen)

// TODO: remove all this old code and replace with meaningful names
// Switch position and size
#define FRAME_X 2
#define FRAME_Y 190
#define FRAME_W 40
#define FRAME_H 30

// Red zone size
#define PREV_BUTTON_X FRAME_X
#define PREV_BUTTON_Y FRAME_Y
#define PREV_BUTTON_W FRAME_W
#define PREV_BUTTON_H FRAME_H

#define NEXT_BUTTON_X (PREV_BUTTON_X + PREV_BUTTON_W + 7)
#define NEXT_BUTTON_Y FRAME_Y
#define NEXT_BUTTON_W (FRAME_W)
#define NEXT_BUTTON_H FRAME_H

// Station change icon/button position (titlebar)
#define STATION_CHANGE_X 278
#define STATION_CHANGE_Y 11

// There are 10 bytes (5 integers @ 2 bytes each) for TFT calibration
#define totCalibrationBytes 10

//TFT_eSPI_Button muteBtn, brightBtn, dimBtn;
TFT_eSPI_Button muteBtn, brightBtn, dimBtn, stnChangeBtn;

// Display routines (hardware/protocol dependent)
void initDisplay()
{
	// NOTE: LITTLEFS must have been mounted prior to calling this helper

	// check if calibration file exists and size is correct
	if (LITTLEFS.exists("/TouchCalData1.txt"))
	{
		File f = LITTLEFS.open("/TouchCalData1.txt", "r");
		if (f)
		{
			// Buffer (integer 2 bytes * 5) to hold calibration data
			uint16_t calData[5];

			// On calibration, 14 bytes were written. TODO: why?
			//if (f.readBytes((char *)calData, 14) == 14)
			if (f.readBytes((char *)calData, totCalibrationBytes) >= totCalibrationBytes){
        log_v("Read %d calibration data bytes", 10);
				tft.setTouch(calData);
			}

			log_v("TFT Calibration data:");
			for (int cnt = 0; cnt < 5; cnt++)
			{
				log_v("%04X, ", calData[cnt]);
			}

			f.close();
			log_i("Touch Calibration completed");
		}
	}
	else
	{
		log_e("NO TOUCH CALIBRATION FILE FOUND");
		displayTrackArtist("NO TOUCH CALIBRATION FILE FOUND");
	}

	// TODO: Change "text" buttons to jpg images with "invisible" buttons (eg black on black) to get the
	// functionality of a button but a better visual experience

	// Change station button (transparent, behind a bmp)
	stnChangeBtn.initButtonUL(
		&tft, STATION_CHANGE_X, STATION_CHANGE_Y, PREV_BUTTON_W, PREV_BUTTON_H, TFT_RED, TFT_RED, TFT_TRANSPARENT, (char *)" ", 1);

	// Brighten the screen
	brightBtn.initButtonUL(
		&tft, NEXT_BUTTON_X + 47, NEXT_BUTTON_Y, NEXT_BUTTON_W, NEXT_BUTTON_H, TFT_YELLOW, TFT_BLUE, TFT_WHITE, (char *)"+", 1);

	// Dim the screen
	dimBtn.initButtonUL(
		&tft, NEXT_BUTTON_X + 93, NEXT_BUTTON_Y, NEXT_BUTTON_W, NEXT_BUTTON_H, TFT_YELLOW, TFT_MAROON, TFT_WHITE, (char *)"-", 1);

	// Black icon, no text
	muteBtn.initButtonUL(
		&tft, 187, FRAME_Y, 57, 30, TFT_BLACK, TFT_BLACK, TFT_BLACK, (char *)"", 1);

	// Run once
	setupDisplayModule(true);
}

// Put the TFT specific code here to prepare your screen, called again on return to HOME screen
void setupDisplayModule(bool initialiseHardWare)
{
	// Initialise
	if (initialiseHardWare)
	{
		tft.init();
	}

	// Rotation as required, the touch calibration MUST match this
	// 0 & 2 Portrait. 1 & 3 landscape
	tft.setRotation(1);

	// Write title of app on screen, using font 2 (x, y, font #)
	tft.fillScreen(TFT_BLACK);

	// Border + fill
	tft.fillRect(1, 1, 318, 43, TFT_RED);
	tft.drawRect(0, 0, 320, 45, TFT_YELLOW);

	// Application Title
	tft.setFreeFont(&FreeSansBold12pt7b);
	tft.setTextSize(1);
	tft.setCursor(26, 30);

	// Set text colour and background
	tft.setTextColor(TFT_YELLOW, TFT_RED);
	tft.println("- ESP32 WEB RADIO -");

	// My details
	tft.setTextFont(2);
	tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
	tft.setCursor(5, tft.height() - 20);
	tft.println("Ralph S Bacon - https://youtube.com/ralphbacon");

	// Draw button(s)
	drawStnChangeButton();
	drawStnChangeBitmap();

	drawBufferLevel(circBuffer.available());

	drawBrightButton(false);
	drawDimButton(false);

	drawMuteButton(false);
	drawMuteBitmap(false);

	// All done
	log_i("TFT Initialised");
	return;
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

// Draw proof-of-concept station change button TODO: expose coordinates
void drawStnChangeButton()
{
	tft.setFreeFont(&FreeSansBold12pt7b);
	stnChangeBtn.setLabelDatum(0, 2);
	stnChangeBtn.drawButton();
}

// Place the "change station" button in the title bar to save space
void drawStnChangeBitmap(bool pressed)
{
	drawBmp(pressed ? "/tuneSmallPressed.bmp":"/tuneSmall.bmp", STATION_CHANGE_X, STATION_CHANGE_Y);
}

// Mute button uses object
void drawMuteButton(bool invert)
{
	//tft.setFreeFont(&FreeSansBold9pt7b);
	//tft.setTextSize(1);
	muteBtn.drawButton(invert);
}

void drawMuteBitmap(bool isMuted)
{
	drawBmp(isMuted ? "/MuteIconOn.bmp" : "/MuteIconOff.bmp", 190, FRAME_Y - 5);
}

void drawBrightButton(bool invert)
{
	tft.setFreeFont(&FreeSansBold12pt7b);
	brightBtn.drawButton(invert);
}

void drawDimButton(bool invert)
{
	tft.setFreeFont(&FreeSansBold12pt7b);
	dimBtn.drawButton(invert);
}

// Draw percentage of buffering (eg 10000 buffer that uses 8000 bytes = 80%)
void drawBufferLevel(size_t bufferLevel, bool override)
{
	static unsigned long prevMillis = millis();
	static int prevBufferPerCent = 0;

	// Only do this infrequently and if the buffer % changes
	if (millis() - prevMillis > 500 || override)
	{
		// Capture current values for next time
		prevMillis = millis();

		// Calculate the percentage (ESP32 has FP processor so shoud be efficient)
		float bufLevel = (float)bufferLevel;
		float arraySize = (float)(CIRCULARBUFFERSIZE);
		int bufferPerCent = (bufLevel / arraySize) * 100.0;

		// Only update the screen on real change (avoids flicker & saves time)
		if (bufferPerCent != prevBufferPerCent || override)
		{
			// Track the buffer percentage
			prevBufferPerCent = bufferPerCent;

			// Print at specific rectangular place
			// TODO: These should not be magic numbers
			uint16_t bgColour, fgColour;
			switch (bufferPerCent)
			{
			case 0 ... 30:
				bgColour = TFT_RED;
				fgColour = TFT_WHITE;
				break;

			case 31 ... 74:
				bgColour = TFT_ORANGE;
				fgColour = TFT_BLACK;
				break;

			case 75 ... 100:
				bgColour = TFT_DARKGREEN;
				fgColour = TFT_WHITE;
				break;

			default:
				bgColour = TFT_WHITE;
				fgColour = TFT_BLACK;
			};

			tft.fillRoundRect(250, FRAME_Y, 60, 30, 5, bgColour);
			tft.drawRoundRect(250, FRAME_Y, 60, 30, 5, TFT_RED);
			tft.setTextColor(fgColour, bgColour);
			tft.setFreeFont(&FreeSans9pt7b);
			tft.setTextSize(1);
			tft.setCursor(261, FRAME_Y + 20);
			tft.printf("%d%%\n", bufferPerCent);
		}
	}
}

// On screen Station Change button (called from main.cpp when TFT touch detected)
// Note that the button is invisible and what you see is a bitmap.
bool getStnChangeButtonPress()
{
	uint16_t t_x = 0, t_y = 0;
	boolean pressed = tft.getTouch(&t_x, &t_y, 50);
	if (pressed && stnChangeBtn.contains(t_x, t_y))
	{
		//log_v("Stn Change: x=%d, y=%d\n", t_x, t_y);
		log_d("Station change button pressed");
		drawStnChangeBitmap(true);

		// While the button is pressed (screen is touched) wait
		while (!digitalRead(tftTouchedPin));
		{
			// give up remainder of this 1mS time slice
			taskYIELD();
		}

		log_d("Station change button released");
		return true;
	}

	return false;
}

// Increase brightness
void getBrightButtonPress()
{
	static unsigned long prevMillis = millis();

	// Only do this infrequently or buffer will stop
	if (millis() - prevMillis > 150)
	{
		uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

		// Update the last time we were here
		prevMillis = millis();

		// Pressed will be set true is there is a valid touch on the screen
		boolean pressed = tft.getTouch(&t_x, &t_y);

		// Check if any key coordinate boxes contain the touch coordinates
		// and highlight the buttons appropriately
		if (pressed && brightBtn.contains(t_x, t_y))
		{
			brightBtn.press(true);
		}
		else
		{
			brightBtn.press(false);
		}

		if (brightBtn.justPressed())
		{
			drawBrightButton(true);
			log_v("Bright!");

			if (prevTFTBright <= 235)
			{
				prevTFTBright += 20;
				ledcWrite(0, prevTFTBright);
			}
		}
		else if (brightBtn.justReleased())
		{
			drawBrightButton(false);
		}

		//Store brightness level in EEPROM
		preferences.putUInt("Bright", prevTFTBright);
	}
}

// Decrease brightness
void getDimButtonPress()
{
	static unsigned long prevMillis = millis();

	// Only do this infrequently or buffer will stop
	if (millis() - prevMillis > 150)
	{
		uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

		// Update the last time we were here
		prevMillis = millis();

		// Pressed will be set true is there is a valid touch on the screen
		boolean pressed = tft.getTouch(&t_x, &t_y);

		// Check if any key coordinate boxes contain the touch coordinates
		if (pressed && dimBtn.contains(t_x, t_y))
		{
			dimBtn.press(true);
		}
		else
		{
			dimBtn.press(false);
		}

		if (dimBtn.justPressed())
		{
			drawDimButton(true);
			log_v("Dim!");

			if (prevTFTBright >= 20)
			{
				prevTFTBright -= 20;
				ledcWrite(0, prevTFTBright);
			}
		}
		else if (dimBtn.justReleased())
		{
			drawDimButton(false);
		}

		//Store brightness level in EEPROM
		preferences.putUInt("Bright", prevTFTBright);
	}
}

// Mute button uses the Button class
static bool isMutedState = false;
void getMuteButtonPress()
{
	static unsigned long prevMillis = millis();

	// Only do this infrequently
	if (millis() - prevMillis > 150)
	{
		uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

		// Update the last time we were here
		prevMillis = millis();

		// Pressed will be set true is there is a valid touch on the screen
		boolean pressed = tft.getTouch(&t_x, &t_y);

		// Check if any key coordinate boxes contain the touch coordinates
		if (pressed && muteBtn.contains(t_x, t_y))
		{
			muteBtn.press(true);
		}
		else
		{
			muteBtn.press(false);
		}

		if (muteBtn.justPressed())
		{
			isMutedState = !isMutedState;

			log_d("Mute: %s", isMutedState ? "Muted" : "UNmuted");
			player.setVolume(isMutedState ? 0 : MAX_VOLUME);
			drawMuteBitmap(isMutedState);
		}
	}
}

void displayStationName(std::string stationName)
{
	// Set text colour and background
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);

	// Clear the remainder of the line from before (eg long title)
	tft.fillRect(0, 56, 320, 40, TFT_BLACK);

	// Write station name (as stored in the sketch)
	tft.setCursor(0, 85);
	tft.setFreeFont(&FreeSansOblique12pt7b);
	tft.setTextSize(1);
	tft.println(stationName.c_str());
}

// Incoming string will normally be in the format Artist - Title
// But some stations do it the other way around. No standards here.
void displayTrackArtist(std::string trackArtistIn)
{
	// Placeholder strings for final output
	std::string justArtist;
	std::string justTitle;

	// Convert string to character array (because that's how I originally wrote this)
	char *trackArtist = (char *)trackArtistIn.c_str();

	// Did we split this string successfully?
	bool splitSuccessful = false;

	// Find where the artist & track are split (if they are)
	char *pointerToDelimiter;
	pointerToDelimiter = strstr(trackArtist, " - ");
	int startCnt = (int)(pointerToDelimiter - trackArtist);

	// If a delimiter was found
	if (pointerToDelimiter != NULL)
	{
		log_v("Found delimiter at position %d", startCnt);

		// Make a new (sub) string of the first part (not including the delimiter)
		std::string justArtistTemp(&trackArtist[0], &trackArtist[startCnt]);
		justArtist = justArtistTemp;
		log_d("Artist: '%s'", justArtist.c_str());

		// Make a new (sub) string of the last part (skipping over the delimiter)
		std::string justTitleTemp(&trackArtist[startCnt + 3], trackArtistIn.size());
		justTitle = justTitleTemp;
		log_d("Title: '%s'", justTitle.c_str());

		// Success
		splitSuccessful = true;
	}
	else
	{
		// Couldn't find a delimiter
		log_v("No delimiter found - using default value");
	}

	// Set default text colour and background
	tft.setTextColor(TFT_GREEN, TFT_BLACK);

	// Clear the remainder of the line from before (eg long title)
	tft.fillRect(0, 100, 320, 80, TFT_BLACK);

	// Write artist / track info
	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setCursor(0, 120);

	// Change colours if we split the string successfully
	if (splitSuccessful)
	{
		// Default colour
		tft.println(justArtist.c_str());

		// Larger font, differentiating colour
		tft.setFreeFont(&FreeSans12pt7b);
		tft.setTextColor(TFT_ORANGE, TFT_BLACK);
		tft.print(justTitle.c_str());
	}
	else
	{
		// Default colour
		tft.print(trackArtistIn.c_str());
	}
}
