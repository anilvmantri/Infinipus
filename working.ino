/*
 * This example shows how to read from a seesaw encoder module.
 * The available encoder API is:
 *      int32_t getEncoderPosition();
        int32_t getEncoderDelta();
        void enableEncoderInterrupt();
        void disableEncoderInterrupt();
        void setEncoderPosition(int32_t pos);
 */
#include <Adafruit_NeoPixel.h>
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

#define SS_SWITCH        24
#define SS_NEOPIX        6

#define SEESAW_ADDR_ONE          0x36
#define SEESAW_ADDR_TWO          0x37
#define SEESAW_ADDR_THREE        0x38

Adafruit_seesaw ssOne;
seesaw_NeoPixel sspixelOne = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);

Adafruit_seesaw ssTwo;
seesaw_NeoPixel sspixelTwo = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);


Adafruit_seesaw ssThree;
seesaw_NeoPixel sspixelThree = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);


Adafruit_NeoPixel stripOne = Adafruit_NeoPixel(350, 4, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripTwo = Adafruit_NeoPixel(350, 5, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripThree = Adafruit_NeoPixel(350, 6, NEO_GRB + NEO_KHZ800);


const int NUM_COLOR_MAPPINGS          = 15;     // Number of colors
int colors[NUM_COLOR_MAPPINGS] = {0x12B8FF, 0x01DC03, 0xFFE62D, 0xFD4499, 0xDF19FB, 0x5E57FF, 0xF23CA6, 0xFF9535, 0x4BFF36, 0x02FEE4, 0xF500EB, 0x0CD4FF, 0x8DFF0A, 0xFFEF06, 0xFF3A06};
int compColors[NUM_COLOR_MAPPINGS] = {0xff5912, 0xdc01da, 0x2d46ff, 0x44fda8, 0x35fb19, 0xf8ff57, 0x3cf288, 0x359fff, 0xea36ff, 0xfe021c, 0x00f50a, 0xff370c, 0x7c0aff, 0x0616ff, 0x06cbff};

long currRainbowHue  = 0; 
int32_t encoder_position_one;
int32_t encoder_position_two;
int32_t encoder_position_three;
long lastSenseTimeMs = 0;
long lastSwapTimeMs = 0;
long lastChangeOne = 0;
long lastChangeTwo = 0;
long lastChangeThree = 0;
bool swapOne = false;
bool swapTwo = false;
bool swapThree = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Looking for seesaw!");
  
  if (! ssOne.begin(SEESAW_ADDR_ONE) || ! sspixelOne.begin(SEESAW_ADDR_ONE)) {
    Serial.println("Couldn't find seesaw 1 on default address");
    while(1) delay(10);
  }

  if (! ssTwo.begin(SEESAW_ADDR_TWO) || ! sspixelTwo.begin(SEESAW_ADDR_TWO)) {
    Serial.println("Couldn't find seesaw 2 on default address");
    while(1) delay(10);
  }

  if (! ssThree.begin(SEESAW_ADDR_THREE) || ! sspixelThree.begin(SEESAW_ADDR_THREE)) {
    Serial.println("Couldn't find seesaw 3 on default address");
    while(1) delay(10);
  }

  uint32_t version = ((ssOne.getVersion() >> 16) & 0xFFFF);
  if (version  != 4991){
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  Serial.println("SS1 Found Product 4991");

  version = ((ssTwo.getVersion() >> 16) & 0xFFFF);
  if (version  != 4991){
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  Serial.println("SS2 Found Product 4991");

  version = ((ssThree.getVersion() >> 16) & 0xFFFF);
  if (version  != 4991){
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  Serial.println("SS3 Found Product 4991");

  // set not so bright!
  sspixelOne.setBrightness(20);
  sspixelOne.show();

  sspixelTwo.setBrightness(20);
  sspixelTwo.show();

  sspixelThree.setBrightness(20);
  sspixelThree.show();
  
  // use a pin for the built in encoder switch
  ssOne.pinMode(SS_SWITCH, INPUT_PULLUP);

  // get starting position
  encoder_position_one = ssOne.getEncoderPosition();
  encoder_position_two = ssTwo.getEncoderPosition();
  encoder_position_three = ssThree.getEncoderPosition();
  
  stripOne.begin();
  stripOne.setBrightness(50);
  stripOne.show();
  stripOne.rainbow(0);
  stripOne.show();

  stripTwo.begin();
  stripTwo.setBrightness(50);
  stripTwo.show();
  stripTwo.rainbow(0);
  stripTwo.show();

  stripThree.begin();
  stripThree.setBrightness(50);
  stripThree.show();
  stripThree.rainbow(0);
  stripThree.show();
  
  Serial.println("Turning on interrupts");
  delay(10);
  ssOne.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  ssOne.enableEncoderInterrupt();
}

void twinkleOne(int currPos) {

    for (int pixel = 0; pixel < stripOne.numPixels(); pixel++)
    {
        if (pixel % 2 == 0)
        {
            if (swapOne)
            {
                stripOne.setPixelColor(pixel, compColors[currPos]);
            }
            else
            {
                stripOne.setPixelColor(pixel, colors[currPos]);
            }
        }
        else
        {
            if (swapOne)
            {
                stripOne.setPixelColor(pixel, colors[currPos]);
            }
            else
            {
                stripOne.setPixelColor(pixel, compColors[currPos]);  
            }
        }
    }

    swapOne = !swapOne;
}

void twinkleTwo(int currPos) {

    for (int pixel = 0; pixel < stripTwo.numPixels(); pixel++)
    {
        if (pixel % 2 == 0)
        {
            if (swapTwo)
            {
                stripTwo.setPixelColor(pixel, compColors[currPos]);
            }
            else
            {
                stripTwo.setPixelColor(pixel, colors[currPos]);
            }
        }
        else
        {
            if (swapTwo)
            {
                stripTwo.setPixelColor(pixel, colors[currPos]);
            }
            else
            {
                stripTwo.setPixelColor(pixel, compColors[currPos]);  
            }
        }
    }

    swapTwo = !swapTwo;
}

void twinkleThree(int currPos) {

    for (int pixel = 0; pixel < stripThree.numPixels(); pixel++)
    {
        if (pixel % 2 == 0)
        {
            if (swapThree)
            {
                stripThree.setPixelColor(pixel, compColors[currPos]);
            }
            else
            {
                stripThree.setPixelColor(pixel, colors[currPos]);
            }
        }
        else
        {
            if (swapThree)
            {
                stripThree.setPixelColor(pixel, colors[currPos]);
            }
            else
            {
                stripThree.setPixelColor(pixel, compColors[currPos]);  
            }
        }
    }

    swapThree = !swapThree;
}

void loop() {
  if (! ssOne.digitalRead(SS_SWITCH)) {
    Serial.println("Button pressed!");
  }

  if (lastSenseTimeMs + 75 < millis())
  {
    int32_t new_position = ssOne.getEncoderPosition();
    if (encoder_position_one != new_position) {
      Serial.println("Encoder 1 new pos:"); 
      Serial.println(new_position);         // display new position

      // change the neopixel color
      // sspixelOne.setPixelColor(0, Wheel(new_position & 0xFF));
      // sspixelOne.show();
      encoder_position_one = new_position;      // and save for next round
      lastChangeOne = millis();
    }

    delay(10);
    new_position = ssTwo.getEncoderPosition();
    if (encoder_position_two != new_position) {
      Serial.println("Encoder 2 new pos:"); 
      Serial.println(new_position);         // display new position

      // change the neopixel color
      // sspixelTwo.setPixelColor(0, Wheel(new_position & 0xFF));
      // sspixelTwo.show();
      encoder_position_two = new_position;      // and save for next round
      lastChangeTwo = millis();
    }

    delay(10);
    new_position = ssThree.getEncoderPosition();
    if (encoder_position_three != new_position) {
      Serial.println("Encoder 3 new pos:"); 
      Serial.println(new_position);         // display new position

      // change the neopixel color
      // sspixelTwo.setPixelColor(0, Wheel(new_position & 0xFF));
      // sspixelTwo.show();
      encoder_position_three = new_position;      // and save for next round
      lastChangeThree = millis();
    }


    lastSenseTimeMs = millis();
  }

  if (lastSwapTimeMs + 50 < millis())
  {
    if (lastChangeOne + 5000 <  millis())
    {
      stripOne.rainbow(currRainbowHue);
    }
    else
    {
      twinkleOne(encoder_position_one % 15);
    }
    stripOne.show();


    if (lastChangeTwo + 5000 <  millis())
    {
      stripTwo.rainbow(currRainbowHue);
    }
    else
    {
      twinkleTwo(encoder_position_two % 15);
    }
    stripTwo.show();


    if (lastChangeThree + 5000 <  millis())
    {
      stripThree.rainbow(currRainbowHue);
    }
    else
    {
      twinkleThree(encoder_position_three % 15);
    }
    stripThree.show();

    lastSwapTimeMs = millis();
    currRainbowHue += 256;
    if (currRainbowHue >= 327680)
    {
        currRainbowHue = 0;
    }
  }

  // don't overwhelm serial port
  delay(10);
}


uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return sspixelOne.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return sspixelOne.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return sspixelOne.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}