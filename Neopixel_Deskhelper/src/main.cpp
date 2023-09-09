
#include <Adafruit_NeoPixel.h>
#include <math.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <credentials.h>
#include <color.h>


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

const byte PIXELDATAPIN   = 0;
const byte NUMBEROFPIXELS = 15;

enum BREATHMODE{IN, OUT};
enum STATES{INIT, IDLE, BREATHE, RUNLEFT, RUNRIGHT, SPLATTER, KITEFFECT, LASTMODE = KITEFFECT, FIRSTMODE = BREATHE};
enum RUN{RUNIN, RUNOUT};
enum INNERSTATE{ENTRY, DO, EXIT};

long currentTime = millis();
long lastTime = currentTime;
long lastStateChangeTime = lastTime;

byte currentBrightness = 0;
byte runIndex = 0;

BREATHMODE BREATH;
STATES STATE;
RUN RUNMODE;
INNERSTATE INSTATE;

Adafruit_NeoPixel pixels(NUMBEROFPIXELS, PIXELDATAPIN, NEO_GRB + NEO_KHZ800); //Constructor, 3rd parameter editable


//const char* ssid = "YOUR-SSID";
//const char* password = "YOUR-PWD";

ESP8266WebServer server(80);


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
void pixelsetup();
void wifiSetup();
void serversetup();

void checkWiFi();

void changeState(long timeInms);
void nextState();
void runPixelsLeftToRight(byte r, byte g, byte b, int delayTime, byte brightness = 200);
void runPixelsRightToLeft(byte r, byte g, byte b, int delayTime, byte brightness = 200);
void setAllPixels(byte r, byte g, byte b, byte brightness);
void setPixels(byte r, byte g, byte b, byte brightness, byte fromPixel = 5, byte pixelCount = 3);
void breathe(byte r, byte g, byte b, int delayTime, byte minBrightness = 5, byte maxBrightness = 50);
void kitEffect(byte r = 255, byte g = 0, byte b = 0, byte brightness = 255, int delayTime = 200);
void splatter(byte brightness = 255, int delayTime = random(30, 300));

void httpBreatheReq();
void httpRunRightReq();
void httpRunLeftReq();

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void setup() {
  // put your setup code here, to run once:

  pixelsetup();
  //wifiSetup();    // enable when using WiFi
  //serversetup();  // enable when using http requests to change through modes

}

void loop() {
  server.handleClient();

  switch (STATE) {
    case INIT :
      STATE = BREATHE;
      break;

    case BREATHE :
      breathe(130, 0, 155, 50);
      break;

    case RUNRIGHT :
      runPixelsLeftToRight(150, 150, 150, 100);
      break;

    case RUNLEFT :
      runPixelsRightToLeft(0, 0, 150, 100);
      break;

    case KITEFFECT :
      kitEffect();
      break;

    case SPLATTER :
      splatter();
      break;
  }

  changeState(20000); //disable if you dont want to cycle modes 

}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

/**
 * @brief handle neopixel setup
*/
void pixelsetup()
{
  pixels.begin();
  setAllPixels(255, 255,255,255); // max. white to show bootup
  delay(500);
  pixels.clear();
}

/**
 * @brief handle server setup, connect http requests to functions that are then called
*/
void serversetup()
{
  // different possible http requests and which function to call on request
  server.on("/breathe", HTTP_GET, httpBreatheReq);
  server.on("/runright", HTTP_GET, httpRunRightReq);
  server.on("/runleft", HTTP_GET, httpRunLeftReq);
  server.begin();
}

/**
 * @brief handle wifi setup 
*/
void wifiSetup()
{
  WiFi.mode(WIFI_STA);        //station mode
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) {
    setPixels(0, 0, 255, 10, 5, 3); // blue for waiting for connection
    delay(100);
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    setAllPixels(0, 255, 0, 100); // green, connection established
    delay(1000);
  }
}

/**
 * @brief check if connection is still up, reconnect if down
*/
void checkWiFi() 
{
  if(WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect();
    delay(500);

    WiFi.begin();

    while (WiFi.status() != WL_CONNECTED) {
      setPixels(0, 0, 255, 10, 5, 3);
      delay(100);
    }
  }
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
/**
 * @brief simply cycle through the states of different lightning modes
 * @param timeInms - time between two changes of states
*/
void changeState(long timeInms) 
{
  if(millis() - lastStateChangeTime > timeInms)
  {
    nextState();
    lastStateChangeTime = millis();
  }
}

/**
 * @brief move state to next state, if last state is achieved, move to first lighning mode state again
*/
void nextState()
{
  if (STATE == LASTMODE)
  {
    STATE = FIRSTMODE;
  }
  else
  {
    STATE = (STATES)((int)STATE + 1);
  }
  
  pixels.clear();
  runIndex = 1;
}

/**
 * @brief fill strip and empty it again, from left to right
 * @param r - red value from 0 to 255
 * @param g - green value from 0 to 255
 * @param b - blue value from 0 to 255
 * @param delayTime - integer value on how fast the strip should fill and empty
 * @param brightness brighness of neopixels
*/
void runPixelsLeftToRight(byte r, byte g, byte b, int delayTime, byte brightness)
{
  pixels.setBrightness(brightness);
  if((millis() - lastTime) > delayTime)
  {

    if(RUNMODE == RUNIN)
    {
      pixels.setPixelColor(runIndex, r, g, b);
      pixels.show();
      runIndex++;
      lastTime = millis();
    }

    if(RUNMODE == RUNOUT)
    {
      pixels.setPixelColor(runIndex, 0, 0, 0);
      pixels.show();
      runIndex++;
      lastTime = millis();
    }

    if(runIndex >= (NUMBEROFPIXELS))
    {
      RUN nextRUNMODE;
      runIndex = 0;
      if(RUNMODE == RUNIN){nextRUNMODE = RUNOUT;}
      else if(RUNMODE == RUNOUT){nextRUNMODE = RUNIN;}

      RUNMODE = nextRUNMODE;
      lastTime = millis();
    }
  }
}

/**
 * @brief fill strip and empty it again, from right to left
 * @param r - red value from 0 to 255
 * @param g - green value from 0 to 255
 * @param b - blue value from 0 to 255
 * @param delayTime - integer value on how fast the strip should fill and empty
 * @param brightness brighness of neopixels
*/
void runPixelsRightToLeft(byte r, byte g, byte b, int delayTime, byte brightness)
{
  pixels.setBrightness(brightness);
  if((millis() - lastTime) > delayTime)
  {

    if(RUNMODE == RUNIN)
    {
      pixels.setPixelColor(runIndex, r, g, b);
      pixels.show();
      runIndex--;
      lastTime = millis();
    }

    if(RUNMODE == RUNOUT)
    {
      pixels.setPixelColor(runIndex, 0, 0, 0);
      pixels.show();
      runIndex--;
      lastTime = millis();
    }

    if(runIndex <= 0)
    {
      RUN nextRUNMODE;
      runIndex = NUMBEROFPIXELS;
      if(RUNMODE == RUNIN){nextRUNMODE = RUNOUT;}
      else if(RUNMODE == RUNOUT){nextRUNMODE = RUNIN;}

      RUNMODE = nextRUNMODE;
      lastTime = millis();
    }
  }
}

/**
 * @brief breathing effect of whole strip
 * @param r - red value from 0 to 255
 * @param g - green value from 0 to 255
 * @param b - blue value from 0 to 255
 * @param delayTime - control speed of breathing effect
 * @param minBrightness - low value of brighness on exhale
 * @param maxBrightness - high value of brightness on inhale
*/
void breathe(byte r, byte g, byte b, int delayTime, byte minBrightness, byte maxBrightness)
{
  if(millis() - lastTime > delayTime)
  {
    (BREATH == IN) ? currentBrightness++ : currentBrightness--;

    if(currentBrightness == maxBrightness){ BREATH = OUT;}
    if(currentBrightness == minBrightness){ BREATH = IN;}

    setAllPixels(r, g, b, currentBrightness);
    lastTime = millis();
  }
}


/**
 * @brief fill strip with color and brightness
 * @param r - red, 0-255
 * @param g - green, 0-255
 * @param b - blue, 0-255
 * @param brightness - brighnessvalue from 0-255
*/
void setAllPixels(byte r, byte g, byte b, byte brightness)
{
  pixels.fill(pixels.Color(r, g, b), 0, 15);
  pixels.setBrightness(brightness);
  pixels.show();
}

/**
 * @brief fill pixel-row with color and brightness
 * @param r - red, 0-255
 * @param g - green, 0-255
 * @param b - blue, 0-255
 * @param brightness - brighnessvalue from 0-255
 * @param fromPIxel - starting pixel
 * @param pixelCount - how many pixels should be filled from fromPixel upwards
*/
void setPixels(byte r, byte g, byte b, byte brightness, byte fromPixel, byte pixelCount)
{
  pixels.fill(pixels.Color(r, g, b), fromPixel, pixelCount);
  pixels.setBrightness(brightness);
  pixels.show();
}

/**
 * @brief imitates the legendary KIT effect
 * @param r - red, 0-255
 * @param g - green, 0-255
 * @param b - blue, 0-255
 * @param brightness - brighnessvalue from 0-255
 * @param delayTime time between switching from led to led
*/
void kitEffect(byte r, byte g, byte b, byte brightness, int delayTime)
{
  if(millis() - lastTime > delayTime)
  {
    pixels.setBrightness(brightness);
    pixels.clear();
    pixels.setPixelColor(runIndex, r, g, b);
    (RUNMODE == RUNIN) ? runIndex++ : runIndex--;
    pixels.show();
    if(runIndex == NUMBEROFPIXELS-1){ RUNMODE = RUNOUT; }
    if(runIndex == 0){ RUNMODE = RUNIN; }
    lastTime = millis();
  }
  
}

/**
 * @brief splatter function, randomly turns led on on random color
 * @param brightness brightness of leds
 * @param delayTime delay until next led is turned on
*/
void splatter(byte brightness, int delayTime)
{
  if(millis() - lastTime > delayTime)
  {
    pixels.setBrightness(brightness);
    pixels.setPixelColor(random(0, NUMBEROFPIXELS), random(0, 255), random(0, 255), random(0, 255));
    pixels.show();
    lastTime = millis();
  }
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-


void httpBreatheReq()
{
  STATE = BREATHE;
  server.send(200);
}

void httpRunRightReq()
{
  STATE = RUNRIGHT;
  server.send(200);
}

void httpRunLeftReq()
{
  STATE = RUNLEFT;
  server.send(200);
}