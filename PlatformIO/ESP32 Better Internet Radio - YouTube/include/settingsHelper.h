#include "Arduino.h"
#include "main.h"

// Cancel/Done button (return to HOME screen)
TFT_eSPI_Button settingsCancelBtn;

#define AUDIO_X 180
#define BASSBOOST_Y 25
#define BASSFREQ_Y 65
#define TREBLEBOOST_Y 105
#define TREBLEFREQ_Y 145
#define AUDIO_OFFSET_Y -14

void displaySettingsScreen()
{
	tft.fillScreen(TFT_BLACK);
	drawSettingsCancelButton();

	//Labels
	tft.setCursor(0, BASSBOOST_Y);
	tft.print("BASS BOOST:");

	tft.setCursor(0, BASSFREQ_Y);
	tft.print("BASS FREQ:");

	tft.setCursor(0, TREBLEBOOST_Y);
	tft.print("TREBLE BOOST:");

	tft.setCursor(0, TREBLEFREQ_Y);
	tft.print("TREBLE FREQ:");

	// Values
	tft.setTextDatum(TR_DATUM);
	tft.setTextColor(TFT_YELLOW);
	tft.setFreeFont(&FreeSans12pt7b);


	//tft.setCursor(AUDIO_X,BASSFREQ_Y);
	//tft.print("30");
	tft.drawCentreString("10", AUDIO_X, BASSBOOST_Y + AUDIO_OFFSET_Y, 1);

	//tft.setCursor(AUDIO_X, BASSFREQ_Y);
	//tft.print("40");
	tft.drawCentreString("40", AUDIO_X, BASSFREQ_Y + AUDIO_OFFSET_Y, 1);

	//tft.setCursor(AUDIO_X, TREBLEBOOST_Y);
	//tft.print(80);
	tft.drawCentreString("6", AUDIO_X, TREBLEBOOST_Y + AUDIO_OFFSET_Y, 1);

	//tft.setCursor(AUDIO_X, TREBLEFREQ_Y);
	//tft.print(100);
	tft.drawCentreString("180", AUDIO_X, TREBLEFREQ_Y + AUDIO_OFFSET_Y, 1);
}

// Settings "Cancel" button
void drawSettingsCancelButton()
{
	tft.setFreeFont(&FreeSans9pt7b);
	settingsCancelBtn.setLabelDatum(0, -3, TC_DATUM);
	settingsCancelBtn.initButtonUL(&tft, 10, 193, 30, 26, TFT_YELLOW, TFT_BLUE, TFT_YELLOW, (char *)"X", 1);
	settingsCancelBtn.drawButton(false);
}

// Exit the settings screen
bool getSettingsCancelPress()
{
	if (getBtnPressAndRelease(settingsCancelBtn))
	{
		return true;
	}

	return false;
}

// Change audio / screen settings
void getSettingsBtn()
{
	if (getBtnPressAndRelease(settingsBtn))
	{
		log_d("Settings button pressed");
		currDisplayScreen = SETTINGS;
		displaySettingsScreen();

		while (1)
		{
			if (getSettingsCancelPress())
			{
				break;
			}
		}

		// Redraw the home screen (banner, station name etc)
		currDisplayScreen = HOME;
		setupDisplayModule(false);
		changeStation(currStnNo);
	}
}