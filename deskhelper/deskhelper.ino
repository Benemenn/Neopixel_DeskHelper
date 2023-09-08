#include <Adafruit_NeoPixel.h>
#include <math.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include <ESPAsyncWebServer.h> //to receive post requests

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

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void runPixelsLeftToRight(byte r, byte g, byte b);
void runPixelsRightToLeft(byte r, byte g, byte b);
void setAllPixels(byte r, byte g, byte b, byte brightness);
void breathe(byte r, byte g, byte b, int delayTime, byte minBrightness, byte maxBrightness);
void breathe(byte r, byte g, byte b, int delayTime);

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void setup() {
  // put your setup code here, to run once:

  pixelsetup();
  serversetup();


}

void loop() {
  switch (STATE) {
    case INIT :
      STATE = IDLE;
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
}

void serversetup()
{
  
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
}

void runPixelsRightToLeft(byte r, byte g, byte b)
{
  for( auto i  = NUMBEROFPIXELS - 1; i >= 0; --i)
  {
    pixels.setPixelColor(i, r, g, b);
    pixels.show();
    delay(500);
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
  breathe(r, g, b, delayTime, 20, 80);
}

void setAllPixels(byte r, byte g, byte b, byte brightness)
{
  pixels.fill(pixels.Color(r, g, b), 0, 15);
  pixels.setBrightness(brightness);
  pixels.show();
}