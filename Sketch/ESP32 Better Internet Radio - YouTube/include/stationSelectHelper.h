#include "Arduino.h"
#include "main.h"
#include "LITTLEFS.h"
#include <vector>

// Display / TFT
#include <User_Setup.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"

// Maximum stations per screen
#define MAX_STATIONS_PER_SCREEN 5

// Max stations and pages calculated in setup()
uint8_t TOTAL_STATIONS;
uint8_t MAX_PAGES;

// Button per row (a button object created per station row for easy access)
TFT_eSPI_Button row[MAX_STATIONS_PER_SCREEN];

// Prev/Next buttons
TFT_eSPI_Button prevBtn;
TFT_eSPI_Button nextBtn;

// Cancel button (return to HOME screen w/o changing anything)
TFT_eSPI_Button stationCancelBtn;

// Which page (of stations) are we displaying?
int currPage = 0;

// Is a button press pending (to be cleared)?
bool buttonPressPending = false;

// The definition for the radio station
typedef struct radioStationLayout
{
	std::string host;
	std::string path;
	uint16_t port;
	std::string friendlyName;
	uint8_t useMetaData;

	// Requires a ctor as we're using emplace_back in the vector manipulation
	// Only required if manually added an initialiser list (as in debugging)
	// radioStationLayout(
	// 	std::string myhost,
	// 	std::string mypath,
	// 	int myport,
	// 	std::string myfriendlyName,
	// 	uint8_t myuseMetadata)
	// 	: host(myhost),
	// 	  path(mypath),
	// 	  port(myport),
	// 	  friendlyName(myfriendlyName),
	// 	  useMetaData(myuseMetadata)
	// {
	// 	// Nothing to do here
	// }
} radioStationLayout_t;

// Create the vector (C++ array) to hold the station names, ports, paths etc
// populated from the SPIFFS file stationList.txt
std::vector<radioStationLayout> radioStation;

// Forward declarations
void listStationsOnScreen(int startPage);
void stationNextPrevButtonPressed();
void drawPrevStationBtn(bool disable = false);
void drawNextStationBtn(bool disable = false);
int getStationListPress();
void addStationToArray(radioStationLayout_t radioStationLayout);
void populateStationList();
void initStationSelect();
void drawStationCancelButton();

void stationSelectSetup()
{
	// Populate the station list vector (array) with stationList.txt
	populateStationList();

	// Count number of stations in list, starting from 1 (not zero)
	TOTAL_STATIONS = radioStation.size();
	log_i("Total stations found: %d", TOTAL_STATIONS);

	// Maximum # of pages of stations
	MAX_PAGES = (1 + ((TOTAL_STATIONS - 1) / MAX_STATIONS_PER_SCREEN));
	log_d("This will require %d pages", MAX_PAGES);
}

void initStationSelect()
{
	// Draw the radio station "buttons" list
	tft.fillScreen(TFT_BLACK);
	listStationsOnScreen(currPage);
}

void listStationsOnScreen(int startPage)
{
	// Generate  X rows for the radio stations
	log_v("Page: %d", startPage);

	// Page X of Y
	tft.setTextSize(1);
	tft.setTextColor(TFT_YELLOW);

	// Increase all counts as we don't count from zero
	char pageCounter[13];
	sprintf(pageCounter, "Page %d of %d", startPage + 1, MAX_PAGES);

	// Text area must be cleared first
	tft.fillRect(100, 205, 100, 35, TFT_BLACK);
	tft.drawCentreString(pageCounter, 160, 205, 2);

	// Clear area of screen with stations listed
	tft.fillRect(0, 0, 320, 200, TFT_BLACK);
	tft.setCursor(0, 0);

	// Offset - which [index] to start displaying the station list from
	int listOffset = startPage * MAX_STATIONS_PER_SCREEN;

	for (auto cnt = listOffset; cnt < listOffset + MAX_STATIONS_PER_SCREEN; cnt++)
	{
		// Any more stations to display for this page (might stop half way through)?
		if (cnt == (TOTAL_STATIONS))
		{
			log_v("No more stations, exiting.");
			break;
		}

		String currStationName = (radioStation[cnt].friendlyName).c_str();
		log_v("Station: %d - %s", cnt, radioStation[cnt].friendlyName.c_str());

		// Calc x-coordinate of each button (y). Calculate nth item in page.
		int nthRow = cnt - (startPage * MAX_STATIONS_PER_SCREEN);
		int startRowAt = nthRow * 35;
		log_v("Nth row: %d", nthRow);

		// Print the "button" for the radio station - highlight the current station for ease of identification
		row[nthRow]
			.initButtonUL(
				&tft, 10, startRowAt, 310, 30, TFT_YELLOW,
				cnt == currStnNo ? TFT_CYAN : TFT_BLUE,
				cnt == currStnNo ? TFT_BLUE : TFT_WHITE,
				(char *)">", 1);

		row[nthRow].setLabelDatum(8, 8, L_BASELINE);
		tft.setFreeFont(&FreeSans9pt7b);
		row[nthRow].drawButton(false, String(cnt + 1) + " - " + currStationName);
	}

	// Update current page
	currPage = startPage;

	// If there is not a previous or next page, "disable" those buttons
	if (currPage == 0)
	{
		drawPrevStationBtn(true);
	}
	else
	{
		drawPrevStationBtn();
	}

	if (currPage + 1 == MAX_PAGES)
	{
		drawNextStationBtn(true);
	}
	else
	{
		drawNextStationBtn();
	}

	// Cancel btn
	drawStationCancelButton();
}

// Next/previous buttons
void drawNextStationBtn(bool disable)
{
	tft.setFreeFont(&FreeSans9pt7b);
	nextBtn.setLabelDatum(-5, 8, L_BASELINE);
	if (disable)
	{
		nextBtn.initButtonUL(&tft, 289, 200, 30, 30, TFT_LIGHTGREY, TFT_DARKGREY, (uint16_t)0xCCCCCC, (char *)">", 1);
	}
	else
	{
		nextBtn.initButtonUL(&tft, 289, 200, 30, 30, TFT_YELLOW, TFT_GREEN, TFT_BLACK, (char *)">", 1);
	}
	nextBtn.drawButton(false);
}

// Next/previous buttons
void drawPrevStationBtn(bool disable)
{
	tft.setFreeFont(&FreeSans9pt7b);
	prevBtn.setLabelDatum(-5, 8, L_BASELINE);
	if (disable)
	{
		prevBtn.initButtonUL(&tft, 10, 200, 30, 30, TFT_LIGHTGREY, TFT_DARKGREY, (uint16_t)0xCCCCCC, (char *)"<", 1);
	}
	else
	{
		prevBtn.initButtonUL(&tft, 10, 200, 30, 30, TFT_YELLOW, TFT_RED, TFT_WHITE, (char *)"<", 1);
	}
	prevBtn.drawButton(false);
}

// Cancel button
void drawStationCancelButton()
{
	tft.setFreeFont(&FreeSansBold9pt7b);
	stationCancelBtn.setLabelDatum(-5, 10, L_BASELINE);
	stationCancelBtn.initButtonUL(&tft, 45, 203, 25, 25, TFT_YELLOW, TFT_BLUE, TFT_YELLOW, (char *)"X", 1);
	stationCancelBtn.drawButton(false);
}

void stationNextPrevButtonPressed()
{
	static unsigned long prevMillis = 0;
	static bool prevBtnPressed = false;
	static bool nextBtnPressed = false;

	// Only allow button presses infrequently
	if (millis() - prevMillis > 100)
	{
		prevMillis = millis();

		// XY coordinates for button presses
		uint16_t x, y;

		// Was a button pressed?
		bool pressed = tft.getTouch(&x, &y, 30);

		// Screen was touched somewhere
		if (pressed)
		{
			// Next page? Not already pressed (must release between presses)
			if (!nextBtnPressed && nextBtn.contains(x, y))
			{

				log_v("Next page!");
				nextBtnPressed = true;

				// Is there a next page to show?
				if (currPage + 1 < MAX_PAGES)
				{
					// Draw button inverted to give user feedback
					nextBtn.drawButton(true);
					listStationsOnScreen(currPage + 1);
				}
				else
				{
					log_v("No Next Page!");
				}
			}

			// Previous button same as above
			if (!prevBtnPressed && prevBtn.contains(x, y))
			{
				log_v("Prev page!");
				prevBtnPressed = true;

				if (currPage - 1 >= 0)
				{
					prevBtn.drawButton(true);
					listStationsOnScreen(currPage - 1);
				}
				else
				{
					log_v("No Prev Page!");
				}
			}
		}
		else
		// Nothing touched on screen so clear previous press(es) if set
		{
			if (nextBtnPressed)
			{
				// Clear button press
				nextBtnPressed = false;

				// Draw the button normally (not inversed)
				nextBtn.drawButton(false);
				log_v("Next Stn Btn released");
			}

			if (prevBtnPressed)
			{
				// As next btn above
				prevBtnPressed = false;
				prevBtn.drawButton(false);
				log_v("Prev Stn Btn released");
			}
		}
	}
}

// Station has been selected?
int getStationListPress()
{
	static unsigned long prevMillis = 0;
	int pressedBtn = -1;

	// Only allow button presses infrequently
	if (millis() - prevMillis > 100)
	{
		prevMillis = millis();

		// XY coordinates for button presses
		uint16_t x, y;

		// Was a button pressed?
		bool pressed = tft.getTouch(&x, &y, 30);

		if (pressed)
		{
			// One of the listed stations pressed?
			for (auto cnt = 0; cnt < MAX_STATIONS_PER_SCREEN; cnt++)
			{
				int stnNameOnScreen = currPage * MAX_STATIONS_PER_SCREEN + cnt;
				if (stnNameOnScreen < radioStation.size())
				{
					log_v("Checking button %d (cnt=%d) of %d for press", stnNameOnScreen, cnt, radioStation.size());

					// See if we "clicked" an on-screen station button
					if (row[cnt].contains(x, y))
					{
						log_d("Pressing button %d\n", cnt);
						log_d("HOST: %s", radioStation[stnNameOnScreen].host.c_str());
						log_d("PATH: %s", radioStation[stnNameOnScreen].path.c_str());
						log_d("PORT: %d", radioStation[stnNameOnScreen].port);
						log_d("NAME: %s", radioStation[stnNameOnScreen].friendlyName.c_str());
						log_d("DATA: %s", radioStation[stnNameOnScreen].useMetaData ? "Yes" : "No");

						row[cnt].press(true);

						String currStationName = (radioStation[stnNameOnScreen].friendlyName).c_str();
						row[cnt].drawButton(true, String(currPage * MAX_STATIONS_PER_SCREEN + cnt + 1) + " - " + currStationName);
						pressedBtn = stnNameOnScreen;
					}
					else
					{
						row[cnt].press(false);
					}
				}
			}
		}
		else
		{
			// FIXME: Button releasing is not required in the main Web Radio code (it just returns to HOME screen)
			// This is being executed but nothing is on screen to "unclick" the button. Needs to be removed.
			if (buttonPressPending)
			{
				log_v("Releasing buttons");
				for (auto cnt = 0; cnt < MAX_STATIONS_PER_SCREEN; cnt++)
				{
					int stnNameOnScreen = currPage * MAX_STATIONS_PER_SCREEN + cnt;
					if (stnNameOnScreen < radioStation.size())
					{
						log_v("Checking button %d of %d for release", stnNameOnScreen, radioStation.size());

						row[cnt].press(false);
						if (row[cnt].justReleased())
						{
							log_v("Releasing button %d", cnt);
							String currStationName = (radioStation[stnNameOnScreen].friendlyName).c_str();
							row[cnt].drawButton(false, String(currPage * MAX_STATIONS_PER_SCREEN + cnt + 1) + " - " + currStationName);
						}
					}
				}
				buttonPressPending = false;
			}
		}

		// Cancel button pressed? Same as pressing current station entry
		if (stationCancelBtn.contains(x, y)){
			pressedBtn = currStnNo;	
		}
	}

	// Nothing pressed that was valid
	return pressedBtn;
}

// Read list of stions from text file and populate a vector (C++ array)
void populateStationList()
{
	if (LITTLEFS.exists("/stationList.txt"))
	{
		File f = LITTLEFS.open("/stationList.txt", "r");
		if (f)
		{
			radioStationLayout_t radioStationLayout;
			char buffer[150] = {'\0'};
			int bufCnt = 0;

			// Create repository for all the parms for this station
			int fieldCnt = 0;

			// For each parameter
			while (f.available())
			{
				// Read each characater
				char fByte = f.read();
				//Serial.printf("%c", fByte);

				// If the character is a quote (or 2nd char of CR/LF pair) ignore it
				if (fByte != '"' && fByte != 10)
				{
					// If the character is NOT a comma add to buffer
					if (fByte != ',' && fByte != 13)
					{
						buffer[bufCnt] = fByte;
						bufCnt++;
					}
					else
					{
						// We've detected a comma, or end-of-line indicates parameter end marker
						log_v("Parameter:'%s'", buffer);

						// Update struct
						switch (fieldCnt)
						{
						case 0:
							radioStationLayout.host = buffer;
							fieldCnt++;
							break;
						case 1:
							radioStationLayout.port = atoi(buffer);
							fieldCnt++;
							break;
						case 2:
							radioStationLayout.path = buffer;
							fieldCnt++;
							break;
						case 3:
							radioStationLayout.friendlyName = buffer;
							fieldCnt++;
							break;
						case 4:
							radioStationLayout.useMetaData = atoi(buffer);

							// Store station in array (vector)
							addStationToArray(radioStationLayout);

							// Reset field count ready for next station
							fieldCnt = 0;
						}

						// Station processing complete: reset field count and init buffer
						memset(buffer, '\0', 150);
						bufCnt = 0;
					}
				}
			}

			// When the data is terminated, add the final one to the vector (array)
			// BUG: Fix this stupid error reading data and finding an EOF at the end of a line
			if (radioStationLayout.host != "")
			{
				radioStationLayout.useMetaData = atoi(buffer);
				addStationToArray(radioStationLayout);
			}

			f.close();
			log_i("Radio stations imported successfully.");

			for (int cnt = 0; cnt < radioStation.size(); cnt++)
			{
				log_d(
					"%d - %s %s %d (%s) %s",
					cnt,
					radioStation[cnt].host.c_str(),
					radioStation[cnt].path.c_str(),
					radioStation[cnt].port,
					radioStation[cnt].friendlyName.c_str(),
					radioStation[cnt].useMetaData == 1 ? "YES" : "NO");
			}
		}
	}
	else
	{
		log_e("NO STATION LIST FILE FOUND");
		displayTrackArtist("No station list (text file) found");
		while (1)
			;
	}
}

void addStationToArray(radioStationLayout_t radioStationLayout)
{
	if (radioStationLayout.friendlyName != "")
	{
		radioStation.emplace_back(radioStationLayout);
		log_v("Vector size:%d", radioStation.size());
	}
}