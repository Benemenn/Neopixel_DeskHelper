
#include <Adafruit_NeoPixel.h>
#include <math.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>



// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

const byte PIXELDATAPIN   = 0;
const byte NUMBEROFPIXELS = 15;

enum BREATH{IN, OUT};
enum STATE{INIT, IDLE, BREATHE, RUNLEFT, RUNRIGHT, FLASH};
enum RUN{RUNIN, RUNOUT};

long currentTime = millis();
long lastTime = currentTime;

byte currentBrightness = 0;
byte runIndex = 0;

BREATH BREATH;
STATE STATE;
RUN RUNMODE;

Adafruit_NeoPixel pixels(NUMBEROFPIXELS, PIXELDATAPIN, NEO_GRB + NEO_KHZ800); //Constructor, 3rd parameter editable



const char* ssid = "YOUR-SSID";
const char* password = "YOUR-PWD";

ESP8266WebServer server(80);


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void runPixelsLeftToRight(byte r, byte g, byte b, int delayTime);
void runPixelsRightToLeft(byte r, byte g, byte b, int delayTime);
void setAllPixels(byte r, byte g, byte b, byte brightness);
void breathe(byte r, byte g, byte b, int delayTime, byte minBrightness, byte maxBrightness);
void breathe(byte r, byte g, byte b, int delayTime);

void httpBreatheReq();
void httpRunRightReq();

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void setup() {
  // put your setup code here, to run once:

  pixelsetup();
  wifiSetup();
  serversetup();


}

void loop() {
  server.handleClient();

  switch (STATE) {
    case INIT :
      STATE = RUNRIGHT;
      break;

    case BREATHE :
      breathe(155, 0, 155, 50);
      break;

    case RUNRIGHT :
      runPixelsLeftToRight(150, 150, 150, 100);
      break;

    case RUNLEFT :
      runPixelsRightToLeft(0, 0, 150, 250);
      break;
  }

}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void pixelsetup()
{
  pixels.begin();
  setAllPixels(255, 255,255,255);
  delay(500);
  pixels.clear();
}

void serversetup()
{
  server.on("/breathe", HTTP_GET, httpBreatheReq);
  server.on("/runright", HTTP_GET, httpRunRightReq);
  server.on("/runleft", HTTP_GET, httpRunLeftReq);
  server.begin();
}

void wifiSetup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    setPixel(0, 0, 255, 10);
    delay(200);
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    setAllPixels(0, 255, 0, 100);
    delay(1000);
  }
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void runPixelsLeftToRight(byte r, byte g, byte b, int delayTime)
{
  pixels.setBrightness(50);
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

void runPixelsRightToLeft(byte r, byte g, byte b, int delayTime)
{
  pixels.setBrightness(50);
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

void breathe(byte r, byte g, byte b, int delayTime)
{
  breathe(r, g, b, delayTime, 5, 50);
}

void setAllPixels(byte r, byte g, byte b, byte brightness)
{
  pixels.fill(pixels.Color(r, g, b), 0, 15);
  pixels.setBrightness(brightness);
  pixels.show();
}

void setPixel(byte r, byte g, byte b, byte brightness)
{
  pixels.fill(pixels.Color(r, g, b), 5, 3);
  pixels.setBrightness(brightness);
  pixels.show();
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