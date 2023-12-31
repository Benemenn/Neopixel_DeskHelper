
#include <Adafruit_NeoPixel.h>
#include <math.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <credentials.h>


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

const byte PIXELDATAPIN   = 0;
const byte NUMBEROFPIXELS = 15;

enum BREATHMODE{IN, OUT};
enum STATES{INIT, IDLE, BREATHE, RUNLEFT, RUNRIGHT, SPLATTER, KITEFFECT, LASTMODE = KITEFFECT, FIRSTMODE = BREATHE};
enum RUN{RUNIN, RUNOUT};
enum INNERSTATE{ENTRY, DO, EXIT};
enum OPERATIONMODE{DEMO, MANUAL};

long currentTime = millis();
long lastTime = currentTime;
long lastStateChangeTime = lastTime;

/// @brief Brightness that is transferred via HTTP_POST Request
byte desiredBrightness = 150;

byte currentBrightness = 0;
byte runIndex = 0;

BREATHMODE BREATH;
STATES STATE;
RUN RUNMODE;
INNERSTATE INSTATE;
OPERATIONMODE OPMODE;

Adafruit_NeoPixel pixels(NUMBEROFPIXELS, PIXELDATAPIN, NEO_GRB + NEO_KHZ800); //Constructor, 3rd parameter editable

const String ESPWIFIHOSTNAME = "ESPDeskStand";
const char* SSID = ssid;
const char* PWD = password;

IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
void pixelsetup();
void wifiClientSetup();
void wifiAPSetup();
void serversetup();

void checkWiFi();

void changeState(long timeInms);
void nextState();
void runPixelsLeftToRight(byte r, byte g, byte b, int delayTime, byte brightness = desiredBrightness);
void runPixelsRightToLeft(byte r, byte g, byte b, int delayTime, byte brightness = desiredBrightness);
void setAllPixels(byte r, byte g, byte b, byte brightness);
void setPixels(byte r, byte g, byte b, byte brightness, byte fromPixel = 5, byte pixelCount = 3);
void breathe(byte r, byte g, byte b, int delayTime, byte minBrightness = 5, byte maxBrightness = desiredBrightness);
void kitEffect(byte r = 255, byte g = 0, byte b = 0, byte brightness = desiredBrightness, int delayTime = 100);
void splatter(byte brightness = desiredBrightness, int delayTime = random(30, 300));

void httpBreatheReq();
void httpRunRightReq();
void httpRunLeftReq();
void httpToggleDemo();
void httpKITReq();
void httpSplatterReq();
void httpSetBrightnessReq();
void httpSetCredentials();

void handleNotFound();

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void setup() {
  // put your setup code here, to run once:

  pixelsetup();
  wifiClientSetup();    // enable when using WiFi
  serversetup();  // enable when using http requests to change through modes
  //wifiAPSetup();

}

void loop() {
  server.handleClient();

  switch (STATE) {
    case INIT :
      STATE = BREATHE;
      break;

    case BREATHE :
      breathe(100, 30, 155, 50);
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

  switch (OPMODE)
  {
    case DEMO :
      changeState(20000); //disable if you dont want to cycle modes 
      break;
    
    case MANUAL :
      //dont do anything
      break;

    default:
      break;
  }
  

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
  server.on("/toggleMode", HTTP_GET, httpToggleDemo);
  server.on("/breathe", HTTP_GET, httpBreatheReq);
  server.on("/runright", HTTP_GET, httpRunRightReq);
  server.on("/runleft", HTTP_GET, httpRunLeftReq);
  server.on("/kit", HTTP_GET, httpKITReq);
  server.on("/party", HTTP_GET, httpSplatterReq);

  server.on("/setBrightness", HTTP_POST, httpSetBrightnessReq);

  server.on("/setCredentials", HTTP_POST, httpSetCredentials);

  server.onNotFound(handleNotFound);

  server.begin();
}

/**
 * @brief handle wifi setup 
*/
void wifiClientSetup()
{
  WiFi.hostname(ESPWIFIHOSTNAME.c_str());
  WiFi.mode(WIFI_STA);        //station mode
  WiFi.begin(SSID, PWD);


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

/**
 * @brief configure your ESP as an AP you can connect to
*/
void wifiAPSetup()
{
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(SSID, PWD);
  delay(500);
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

void httpToggleDemo()
{
  OPMODE = ((OPMODE == DEMO) ? MANUAL : DEMO);
  server.send(200);
}

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

void httpKITReq()
{
  STATE = KITEFFECT;
  server.send(200);
}

void httpSplatterReq()
{
  STATE = SPLATTER;
  server.send(200);
}

void httpSetBrightnessReq()
{
  if (! server.hasArg("brightness"))
  {
    server.send(400, "text/plain", "400: Invalid Request");
  }
  else if(server.arg("brightness").toInt() > 255)
  {
    server.send(400, "text/plain", "400: Invalid Request, Value not in bound");
  }
  else
  {
    desiredBrightness = server.arg("brightness").toInt();
    server.send(200);
  }
  
}

void httpSetCredentials()
{
  if (!server.hasArg("auth") || !server.hasArg("ssid") || !server.hasArg("pwd") || server.arg("auth") == NULL || server.arg("ssid") == NULL || server.arg("pwd") == NULL)
  {
    server.send(400, "text/plain", "400: Invalid Request");
  }
  else if(server.arg("auth") == "admin")
  {
    const char* newSSID = (server.arg("ssid")).c_str();
    const char* newPWD = (server.arg("pwd")).c_str();

    byte tryCounter = 0;
    byte maxTryCounter = 15;

    WiFi.disconnect(true);
    pixels.clear();

    WiFi.hostname(ESPWIFIHOSTNAME.c_str());
    WiFi.mode(WIFI_STA);        //station mode
    
    WiFi.begin(newSSID, newPWD);


    while (WiFi.status() != WL_CONNECTED && tryCounter <= maxTryCounter) {
      setPixels(0, 0, 255, 10, 5, 3); // blue for waiting for connection
      delay(1000);
      tryCounter++;
    }

    if (WiFi.status() != WL_CONNECTED && tryCounter > maxTryCounter)
    {
      wifiClientSetup();
    }
    
    if(WiFi.status() == WL_CONNECTED)
    {
      setPixels(0, 255, 0, 255, 5, 1); // green, connection established
      //server.send(200, "text/plain", "200: Connected to Network!"); //original sender cannot receive response, since network has changed
    }
    
  }
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found");
}