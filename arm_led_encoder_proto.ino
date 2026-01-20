#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

#define NUM_ENCODERS 6

int32_t addrs[NUM_ENCODERS] = {0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B};
int32_t positions[NUM_ENCODERS];

Adafruit_seesaw ss[NUM_ENCODERS];

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Looking for seesaws!");
  int32_t found = 0;
  for(int32_t addr = 0; addr < NUM_ENCODERS; addr++)
  {
      positions[addr] = 0;
      if (!ss[addr].begin(addrs[addr]))
      {
        Serial.println("Couldn't find seesaw on address");
        Serial.println(addrs[addr]);
      }
      else
      {
        Serial.println("Seesaw found at address");
        Serial.println(addrs[addr]);
        found += 1;
      }
  }

  if (found != NUM_ENCODERS)
  {
    while(1)
    {
      delay(100);
      Serial.println("Failed to find all seesaws, found:");
      Serial.println(found);
    }
  }

  for (int32_t encoder = 0; encoder < NUM_ENCODERS; encoder++)
  {
    positions[encoder] = 0;
    positions[encoder] = ss[encoder].getEncoderPosition();
            Serial.println("Encoder");
            Serial.println(encoder);
            Serial.println("Start position");
            Serial.println(positions[encoder]);
  }   

  Serial.println("seesaws started");
}

void loop() 
{
    for (int32_t encoder = 0; encoder < NUM_ENCODERS; encoder++)
    {
        int32_t currPos = ss[encoder].getEncoderPosition();
        if (currPos != positions[encoder])
        {
            positions[encoder] = currPos;
            Serial.println("Encoder");
            Serial.println(encoder);
            Serial.println("New position");
            Serial.println(positions[encoder]);
        }

    }
}