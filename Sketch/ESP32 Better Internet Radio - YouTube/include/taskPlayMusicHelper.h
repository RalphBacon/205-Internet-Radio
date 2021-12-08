/* 
	We need the music playing execution to run regardless of what we may be
	doing in the main loop() (such as increasing the screen brightness) all
	of which takes time and can interrupt the playing of music.

	Here we run the actual code to push data to the MP3 decoder (VS1053)
	regardless (well, as long as there is data in the circular buffer - 
	which could be a candidate for another task, but we do have 10 seconds
	to play with).
*/

#include "Arduino.h"
#include "main.h"

// This determines whether the buffer contains sufficient data to allow playing
// It is shared between the main loop() and the task below, hence volatile.
volatile bool canPlayMusicFromBuffer = false;

// Create the task handle (a reference to the task being created later)
TaskHandle_t playMusicTaskHandle;

// This is the task that we will start running (on Core 1, don't use Core 0)
void playMusicTask(void *parameter)
{
	static unsigned long prevMillis = 0;

	// Do this forever
	while (1)
	{
		// If we (no longer) need to buffer the streaming data (after a station change)
		// allow the buffer to be played, but if we get an error stop playing for a while
		if (canPlayMusicFromBuffer)
		{
			// If we failed to play 32b from the buffer (insufficient data) set the flag again
			if (!playMusicFromRingBuffer())
			{
				canPlayMusicFromBuffer = false;
			};

			// We've queued up more music, no more to do this time slice
			taskYIELD();
		}
		else
		{
			// Otherwise, check whether we now have enough data to start playing
			checkBufferForPlaying();
		}

		// We should check that the stack size allocated was correct. This shows the FREE
		// stack space every X minutes. Assuming we have run all paths it should remain constant.
		// Comment out this code once satisfied that we have allocated the correct stack space.
		if (millis() - prevMillis > (60000 * 5))
		{
// Compiler warning about unused variable if we are not logging this value
#if CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_DEBUG
			unsigned long remainingStack = uxTaskGetStackHighWaterMark(NULL);
			log_v("Free stack:%lu", remainingStack);
#endif
			prevMillis = millis();
		}
	}
}

// Read the ringBuffer and give to VS1053 to play
bool playMusicFromRingBuffer()
{
	// Did we run out of data to send the VS1053 decoder?
	bool dataPanic = false;

	// Now read (up to) 32 bytes of audio data and play it
	if (circBuffer.available() >= 32)
	{
		// Does the VS1053 actually want any more data (yet)?
		if (player.data_request())
		{
			{
				// Read (up to) 32 bytes of data from the circular (ring) buffer
				int bytesRead = circBuffer.read((char *)mp3buff, 32);

				// If we didn't read the full 32 bytes, that's a worry!
				if (bytesRead != 32)
				{
					log_e("Only read %db from ring buff", bytesRead);
					dataPanic = true;
				}

				// Actually send the data to the VS1053
				player.playChunk(mp3buff, bytesRead);
			}
		}
	}

	return !dataPanic;
}

// Copy streaming data to our ring buffer and check wheter there's enough to start playing yet
// This is only excuted after a station connect to give the buffer a chance to fill up.
void checkBufferForPlaying()
{
	// If we have now got enough in the ring buffer to allow playing to start without stuttering?
	if (circBuffer.available() > CIRCULARBUFFERSIZE / 4)
	{
		// Reset the flag, allowing data to be played, won't get reset again until station change
		canPlayMusicFromBuffer = true;
	}
}

// Called from the main setup() routine, it sets up the above task and runs it as soon as it
// is declared (so choose your moment wisely)
void taskSetup()
{
	// Independent Task to play music
	xTaskCreatePinnedToCore(
		playMusicTask,		  /* Function to implement the task */
		"WebRadio",			  /* Name of the task */
		1800,				  /* Stack size in words */
		NULL,				  /* Task input parameter */
		1,					  /* Priority of the task - must be higher than 0 (idle)*/
		&playMusicTaskHandle, /* Task handle. */
		1);					  /* Core where the task should run */
}