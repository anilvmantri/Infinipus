/**
 * Master LED Software. Uses strips of NeoPixels as well as an RS485 module to communicate over UART (serial) with (possibly)
 * multiple slaves. Basic loop is two steps:
 * 1.) Get Encoder Values:
 *  - Check if enough time has passed to ask slaves for new data - if not go to coloring
 *  - If so, walk through all slave IDs - set RS485 to transmit & dispatch the ID we want
 *  - Set our RS485 to receive & wait for response
 *  - Check if the given encoder value (response) is different from the previous - then set the RS485 back to transmit
 * 2.) Coloring
 *  - For each attached NeoPixel string, check if there is a new encoder value from its associated slave
 *  - If so, update with new color profile. If not, check time since last new encoder value
 *  - If passed update threshold, change color back to default (rainbow)
 *  - Otherwise, color with last recorded encoder value
 */

 #include <Adafruit_NeoPixel.h>

// Master variables/constants
const int MASTER_DELAY_MS = 6000;
const int timeBetweenChecksMs = 100;
const long timeToWaitMs = 200;
long lastCheck = 0;

// RS485 variables/constants
const int SSERIAL_CTRL_PIN= 3;
const int RS485_TRANSMIT = HIGH;
const int RS485_RECEIVE = LOW;
int byteReceived;

// Encoder variable/constants
const int NUM_ENCODERS = 7;
const char ENCODER_IDS[NUM_ENCODERS] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};

// NeoPixel variables/constants
int encoderPosition[NUM_ENCODERS] = {0, 0, 0, 0, 0, 0, 0};
long lastChanged[NUM_ENCODERS] = {0, 0, 0, 0, 0, 0, 0};
bool encoderExists[NUM_ENCODERS] = {true, true, true, true, true, true, true};
int theaterChaseIndices[NUM_ENCODERS] = {0, 0, 0, 0, 0, 0, 0};
byte randColorSeed[NUM_ENCODERS] = {0, 0, 0, 0, 0, 0, 0};
int stripOneEncoderPos = 0;
long lastChangeOne = 0;
const int STRIP_ONE_DATA_PIN = 17;
const int NUM_LEDS_IN_STRIP = 630; // (~85 pixels per segment * 7 segments per arm ~= 600 pixels per arm)

Adafruit_NeoPixel strips[NUM_ENCODERS] = {
  Adafruit_NeoPixel(NUM_LEDS_IN_STRIP, STRIP_ONE_DATA_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_IN_STRIP, STRIP_ONE_DATA_PIN + 1, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_IN_STRIP, STRIP_ONE_DATA_PIN + 2, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_IN_STRIP, STRIP_ONE_DATA_PIN + 3, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_IN_STRIP,  STRIP_ONE_DATA_PIN + 4, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_IN_STRIP,  STRIP_ONE_DATA_PIN + 5, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_IN_STRIP,  STRIP_ONE_DATA_PIN + 6, NEO_GRB + NEO_KHZ800),
};


// Coloring variables/constants
const int timeToRainbowMs = 5000;
long currRainbowHue  = 0;

void setup()
{
    Serial.begin(9600);
  
    // Setup our RS485 control - putting it into receive mode to start
    pinMode(SSERIAL_CTRL_PIN, OUTPUT);    
    digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);

    for(int i = 0; i < NUM_ENCODERS; i++)
    {
        strips[i].begin();
        strips[i].setBrightness(25);
        strips[i].rainbow(random() % 327680);
        strips[i].show();  
    }
    
    // Setup our TX/RX communication
    Serial1.begin(115200);

    // Delay a small amount of time - so that our slaves can initialize
    delay(MASTER_DELAY_MS);

    Serial.println("Master Ready");
}

uint32_t randColor(byte seed)
{
    seed = 255 - seed;
    if(seed < 85)
    {
        return strips[0].Color(255 - seed * 3, 0, seed * 3);
    }
    else if(seed < 170)
    {
        seed -= 85;
        return strips[0].Color(0, seed * 3, 255 - seed * 3);
    }
    else
    {
        seed -= 170;
        return strips[0].Color(seed * 3, 255 - seed * 3, 0);
    }
}

void read_encoder_value(int encoderIdx)
{
    // Attempt to talk to the given encoder - via encoderId
    digitalWrite(SSERIAL_CTRL_PIN, RS485_TRANSMIT);
    Serial1.write(ENCODER_IDS[encoderIdx]);
    delay(1);
    digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);

    Serial.println("Hello!");
    Serial.println(ENCODER_IDS[encoderIdx]);

    // Wait for a response
    long startTime = millis();
    while(!Serial1.available())
    {
        delay(10);

        // This encoder failed to respond - try a different one & come back later
        if(startTime + timeToWaitMs < millis())
        {
            Serial.println("Failed to communicate with encoder - marking as absent");
          Serial.println(ENCODER_IDS[encoderIdx]);
          Serial.println("---------");
            // encoderExists[encoderIdx] = false;
            break;
        }
    }

    // If the encoder responded - record its position
    if(encoderExists[encoderIdx])
    {
        byteReceived = Serial1.read();
        delay(10);
        if (encoderPosition[encoderIdx] != byteReceived)
        {
            Serial.println("Encoder - New Position");
            Serial.println(ENCODER_IDS[encoderIdx]);
          Serial.println(byteReceived);
          Serial.println("---------");
          byte newColorSeed = random() % 255;
          if(newColorSeed < randColorSeed[encoderIdx] + 70 && newColorSeed > randColorSeed[encoderIdx] - 70)
          {
            randColorSeed[encoderIdx] = random() % 255;
          }
          else
          {
            randColorSeed[encoderIdx] = (randColorSeed[encoderIdx] + 80) % 255;
          }
            encoderPosition[encoderIdx] = byteReceived;
            lastChanged[encoderIdx] = millis();
        }
    }

    delay(10);
}

void rainbow(int encoderIdx)
{
    strips[encoderIdx].rainbow(currRainbowHue);
    strips[encoderIdx].show();
}

void colorFill(int encoderIdx)
{
    strips[encoderIdx].fill(randColor(randColorSeed[encoderIdx]), 0, strips[encoderIdx].numPixels() - 1);
    strips[encoderIdx].show();
}

void theaterChase(int encoderIdx)
{
  for(int pixelIdx = 0; pixelIdx < strips[encoderIdx].numPixels(); pixelIdx++)
  {
      if ((pixelIdx + theaterChaseIndices[encoderIdx]) % 3 == 0)
      {
          strips[encoderIdx].setPixelColor(pixelIdx, randColor(randColorSeed[encoderIdx]));
      }
      else
      {
          strips[encoderIdx].setPixelColor(pixelIdx, randColor((randColorSeed[encoderIdx] + 80) % 255));
      }
  }
  theaterChaseIndices[encoderIdx]++;
  strips[encoderIdx].show();
}

void color_leds(int encoderIdx)
{
    if(encoderExists[encoderIdx])
    {
        // Color our LED strips based on associated slaves last read encoder position
        if ((lastChanged[encoderIdx] + timeToRainbowMs) < millis())
        {
            rainbow(encoderIdx);
        }
        else if (encoderPosition[encoderIdx] % 2 == 0)
        {
          colorFill(encoderIdx);
        }
      else
      {
        theaterChase(encoderIdx);
      }
    }
    else
    {
        // If the encoder doesn't exist, just rainbow it's associated LEDs
        rainbow(encoderIdx);
    }
}

void loop() 
{
    if((lastCheck + timeBetweenChecksMs) < millis())
    {
        for(int encoderIdx = 0; encoderIdx < NUM_ENCODERS; encoderIdx++)
        {
            if (encoderExists[encoderIdx])
            {
                read_encoder_value(encoderIdx);
            }
        }

        lastCheck = millis();
    }

    // // Color our LED strips based on associated slaves last read encoder position
    for(int encoderIdx = 0; encoderIdx < NUM_ENCODERS; encoderIdx++)
    {
        color_leds(encoderIdx);
    }

    // Update global rainbow hue value
    currRainbowHue += 512;
    if (currRainbowHue >= 327680)
    {
        Serial.println("Looping Rainbow!");
        currRainbowHue = 0;
    }
}