# 205 Internet Radio  
### An Internet Streaming Radio using an ESP32, a VS1053 MP3 decoder, and an ILI6341 TFT touch screen  

### See my YouTube video here: https://youtu.be/xrR8EZh2bMI  
An Internet Radio Player for around $20 (without a case) can't be all bad and you get to build it too!  

<p align="center">
<img src="/Arduino IDE Version/ESP32-WROVER_Web_Radio/images/v1.35_Arduino_IDE_version.jpg" width="25%">
  
</p>  
<p align="center"> 
<em>Current version (July 2021)</em>
</p>

**Release V1.35**  
**Fixed:** BBC HTTP 1v1 vs 1v0 problem (chunked data)  
**Updated:** list of BBC stations  
**Fixed:** A few minor background issues, some debugging messages removed or downgraded  
**Fixed:** Title/Artist font reduced to fit in screen if at all possible  
**New:** Settings screen, work in progress, not yet functional

This **Internet Radio** was written from the ground up so that I could learn how it all worked. As it's now version 1.35 I think it's OK to release onto my unsuspecting YouTube viewers!  

Although we don't really go through the code (there's over 900 lines and counting) I do explain the crux of connecting to a streaming radio URL and how we get the data stream - along with  the Artist and Track information, if you want.  

Your comments, requests and improvements gratefully received!  

#### To support my channel please do click on the affiliate links below before ordering ANYTHING from Banggood, AliExpress or Amazon. It expires 24 hours after clicking it so do come back and re-click!  


<img src="https://user-images.githubusercontent.com/20911308/135296246-f216aa5b-0567-4aa8-b1b3-30a3ed92373b.gif" align="left">
<br />
<a href="https://buymeacoffee.com/ralphbacon" target="_blank">You can support me by buying me a coffee!</a>  

-----------  
INFORMATION  
-----------  

**VS1053** Datasheet  
https://cdn-shop.adafruit.com/datasheets/vs1053.pdf  

Calculating data stream duration (eg for how long does 32b of data last?)  
https://www.colincrawley.com/audio-file-size-calculator/  

Bodmer's **TFT_eSPI library** for 32-bit processors (eg ESP32 ESP8266)  
https://github.com/Bodmer/TFT_eSPI  

Baldram's (Marcin Szałomski) **library for the VS1053** that I used   
https://github.com/baldram/ESP_VS1053_Library

-----------------------
INTERNET RADIO STATIONS
-----------------------

List of many **Internet Radio stations**  
https://www.internet-radio.com  

Updated list of **BBC network radio URLs**  
http://www.suppertime.co.uk/blogmywiki/2015/04/updated-list-of-bbc-network-radio-urls/

List of **many UK radio stations**  
http://www.radiofeeds.co.uk/mp3.asp   

**Bauer media** radio stations (eg Magic, Greatest Hits etc)  
http://www.astra2sat.com/bauer-media/bauer-radio-streams/  


### List of all my videos  
(Special thanks to Michael Kurt Vogel for compiling this)  
http://bit.ly/YouTubeVideoList-RalphBacon  

--------
PRODUCTS
--------

The **2.8" TFT touch screen** used in my demo for just $6.66 (plus $2.05 shipping) or from USA & UK direct:  
2.8 Inch ILI9341 240x320 SPI TFT LCD Display Touch Panel SPI Serial Port Module  
https://www.banggood.com/custlink/v33EPJT6cF

**38-pin ESP32 with headers** for just $4.99 plus $2.25 shipping ($9.00 plus 0.99 shipping if ordered from USA):  
https://www.banggood.com/custlink/KGKEHpfeCc  
**NOTE the 30-pin works just as well, the extra pins are not for end-user use anyway!  

**Amazon UK** sell this 2.8" screen (in the UK) for just £12.50 + free delivery!  
https://amzn.to/38utf96  
**NB: Make sure you select the TOUCH version!!!**  

**Amazon UK** also sell the **38-pin ESP32 DEv Kit** for just £5.55 in the UK:  
https://amzn.to/3mFhy30  

**AliExpress offer both 30-pin and 38-pin** ($3.36 + $1.04 shipping) of the ESP32 Dev Kit:  
https://www.aliexpress.com/item/32959541446.html  
I couldn't get an affiliate link for above, click here first: https://s.click.aliexpress.com/e/_ADe0jl
That's a link for a bare ESP32 with a whopping 16Mb on board! But needs an expansion board/PCB.  

My channel, GitHub and blog are here:  
\------------------------------------------------------------------  
https://www.youtube.com/RalphBacon  
https://ralphbacon.blog  
https://github.com/RalphBacon  
\------------------------------------------------------------------
