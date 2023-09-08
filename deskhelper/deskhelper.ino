
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

long currentTime = millis();
long lastTime = currentTime;
byte currentBrightness = 0;

BREATH BREATH;
STATE STATE;

Adafruit_NeoPixel pixels(NUMBEROFPIXELS, PIXELDATAPIN, NEO_GRB + NEO_KHZ800); //Constructor, 3rd parameter editable

const char* ssid = "YOUR-SSID";
const char* password = "YOUR-PASSWORD";

ESP8266WebServer server(80);


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void runPixelsLeftToRight(byte r, byte g, byte b);
void runPixelsRightToLeft(byte r, byte g, byte b);
void setAllPixels(byte r, byte g, byte b, byte brightness);
void breathe(byte r, byte g, byte b, int delayTime, byte minBrightness, byte maxBrightness);
void breathe(byte r, byte g, byte b, int delayTime);

void httpBreatheReq();
void httpRunReq();

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
      runPixelsLeftToRight(150, 150, 150);
      break;

    case RUNLEFT :
      runPixelsRightToLeft(0, 0, 150);
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
  server.on("/run", HTTP_GET, httpRunReq);
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

void runPixelsLeftToRight(byte r, byte g, byte b)
{
  for( auto i  = 0; i < NUMBEROFPIXELS; ++i)
  {
    pixels.setPixelColor(i, r, g, b);
    pixels.show();
    delay(500);
  }
  pixels.clear();
}

void runPixelsRightToLeft(byte r, byte g, byte b)
{
  for( auto i  = NUMBEROFPIXELS - 1; i >= 0; --i)
  {
    pixels.setPixelColor(i, r, g, b);
    pixels.show();
    delay(500);
  }
  pixels.clear();
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
  breathe(r, g, b, delayTime, 20, 80);
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

void httpRunReq()
{
  STATE = RUNRIGHT;
  server.send(200);
}