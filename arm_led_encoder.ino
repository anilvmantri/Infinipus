/**
 * This program is designed to loop through NUM_ARMS rotary encoders (mapped to 
 * the arduino's ports below) reading each & detecting if the value has changed.
 * If so, the corresponding arm's LEDs are set based on a color/complementary color 
 * mapping. If no change has been detected on a given encoder for NUM_SECS_TO_RAINBOW, 
 * the associated arm's LEDs default to rainbow.
 *
 * v0.1 - 2/19/24 - Initial version working on prototype board
 * v0.2 - 4/19/24 - Switched to encoders (vs pots)
 */

#include <Adafruit_NeoPixel.h>
#include "Adafruit_seesaw.h"
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

const int NUM_ARMS                    = 6;      // Number of arms
const int LED_COUNT                   = 350;    // Number of NeoPixels attached to each arm
const int NUM_MS_TO_RAINBOW           = 10000;  // Miliseconds before we go back to rainbowing
const int NUM_MS_BEFORE_SENSOR_REREAD = 1000;   // Miliseconds between each read of all arm input sensors
const int NUM_COLOR_MAPPINGS          = 15;     // Number of colors
const char DEFAULT_BRIGHTNESS         = 50;     // Default value of brightness, max is 255
const long MAX_RAINBOW_HUE            = 327680; // Maximum rainbow hue value
const long RAINBOW_HUE_INC_AMOUNT     = 256;    // Amount to increment our global rainbow hue value by each coloring loop

/**
 * Structure containing an individual arms information
 */
typedef struct
{
    Adafruit_NeoPixel strip; // NeoPixel strip object for this arm
    Adafruit_seesaw encoder; // Adafruit seesaw object (encoder board)
    int currentColorIndex;   // The current color index, based on last input/encoder reading
    int currentEncoderPos;   // The current encoder position
    bool isActive;           // Flag to determine if we're currently rainbowing or not
    long activedTimeMs;      // Last time in miliseconds this arm updated with a new non-rainbow color
    bool swap;               // Variable allowing us to 'twinkle' between each color/comp color
} arm_t;

// Array of all arms covered by this arduino
arm_t arms[NUM_ARMS];

// Array mapping arms -> output pins to LED strip
int outputLedPinMap[NUM_ARMS] = {8, 7, 6, 5, 4, 3};

// Array mapping encoder addresses -> arms
int inputEncoderAddressMap[NUM_ARMS] = {0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B};

// Array mapping encoder index -> color/complementary colors
int colors[NUM_COLOR_MAPPINGS] = {0x12B8FF, 0x01DC03, 0xFFE62D, 0xFD4499, 0xDF19FB, 0x5E57FF, 0xF23CA6, 0xFF9535, 0x4BFF36, 0x02FEE4, 0xF500EB, 0x0CD4FF, 0x8DFF0A, 0xFFEF06, 0xFF3A06};
int compColors[NUM_COLOR_MAPPINGS] = {0xff5912, 0xdc01da, 0x2d46ff, 0x44fda8, 0x35fb19, 0xf8ff57, 0x3cf288, 0x359fff, 0xea36ff, 0xfe021c, 0x00f50a, 0xff370c, 0x7c0aff, 0x0616ff, 0x06cbff};

long currRainbowHue  = 0;    // Global variable for the current rainbow hue to use for all arms rainbowing
long lastSenseTimeMs = 5000; // Last time in miliseconds we read all arms input/encoder values

/**
 * Setup function, run once at power on. Initializes each arm, mapping it
 * via above arrays to an encoder input pin and an LED strip output pin 
 */
void setup () {

  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif

    Serial.begin(115200);

    // Setup each arm in our arms array - including it's input pin
    for(int currArm = 0; currArm < NUM_ARMS; currArm++)
    {
        // Create a NeoPixel strip for this arm's LED output pin mapping
        arms[currArm].strip = Adafruit_NeoPixel(LED_COUNT, outputLedPinMap[currArm], NEO_GRB + NEO_KHZ800);

        // Enable the strip
        arms[currArm].strip.begin();
        arms[currArm].strip.show();
        arms[currArm].strip.setBrightness(DEFAULT_BRIGHTNESS);

        // Set the current color index & default rainbow value
        arms[currArm].currentColorIndex = NUM_COLOR_MAPPINGS + 1;

        // Set isActive to false
        arms[currArm].isActive = false;
        arms[currArm].activedTimeMs = 0;

        // Configure this arm's encoder
        if (arms[currArm].encoder.begin(inputEncoderAddressMap[currArm]))
        {
            // Get this encoders initial position
            arms[currArm].currentEncoderPos = arms[currArm].encoder.getEncoderPosition();
            Serial.println("Encoder found for arm:");
            Serial.println(currArm);
            Serial.println("Address:");
            Serial.println(inputEncoderAddressMap[currArm]);
        }
    }
}

/**
 * Reads all arms encoders, mapped to each arm index via inputEncoderAddressMap
 * every NUM_MS_BEFORE_SENSOR_REREAD miliseconds to determine if we should twinkle
 */
void readEncoders() {

    if (lastSenseTimeMs + NUM_MS_BEFORE_SENSOR_REREAD < millis())
    {
        static int newPossibleColorIndex = 0;
        for(int currArm = 0; currArm < NUM_ARMS; currArm++)
        {
            // Read the input pin associated with this arm - only do the expensive mapping if the encoder has changed position
            int currEncoderPos = arms[currArm].encoder.getEncoderPosition();
            if (currEncoderPos != arms[currArm].currentEncoderPos)
            {
                arms[currArm].currentEncoderPos = currEncoderPos;
                newPossibleColorIndex = map(arms[currArm].currentEncoderPos, 0, 255, 0, NUM_COLOR_MAPPINGS - 1);
            
                // Have less colors than encoder steps, so if this arm has a new value update it, otherwise check if it's time go back to rainbowing
                if (newPossibleColorIndex != arms[currArm].currentColorIndex)
                {
                    arms[currArm].currentColorIndex = newPossibleColorIndex;
                    arms[currArm].isActive = true;
                    arms[currArm].activedTimeMs = millis();
                }
                else if (arms[currArm].activedTimeMs + NUM_MS_TO_RAINBOW < millis())
                {
                    arms[currArm].isActive = false;
                }
            }
            else if (arms[currArm].activedTimeMs + NUM_MS_TO_RAINBOW < millis())
            {
                arms[currArm].isActive = false;
            }

            delay(10);
        }

        lastSenseTimeMs = millis();
    }
}

/**
 * Simple rainbow function, using the current global rainbow hue currRainbowHue
 * 
 * @param currArm The arm that is currently rainbowing
 */
void rainbow(int currArm) {
    arms[currArm].strip.rainbow(currRainbowHue);
}

/**
 * Twinkles the read color/complementary color every even/odd pixel. Swaps even/odd
 * coloring between each call.
 * 
 * @param currArm The arm that is currently twinkling
 */
void twinkle(int currArm) {

    for (int pixel = 0; pixel < arms[currArm].strip.numPixels(); pixel++)
    {
        if (pixel % 2 == 0)
        {
            if (arms[currArm].swap)
            {
                arms[currArm].strip.setPixelColor(pixel, compColors[arms[currArm].currentColorIndex]);
            }
            else
            {
                arms[currArm].strip.setPixelColor(pixel, colors[arms[currArm].currentColorIndex]);
            }
        }
        else
        {
            if (arms[currArm].swap)
            {
                arms[currArm].strip.setPixelColor(pixel, colors[arms[currArm].currentColorIndex]);
            }
            else
            {
                arms[currArm].strip.setPixelColor(pixel, compColors[arms[currArm].currentColorIndex]);  
            }
        }
    }

    arms[currArm].swap = !arms[currArm].swap;
}

/**
 * Colors each arm's strip, based on if it's active (twinkling based on read color) or is
 * default rainbowing.
 *
 * @note The rainbow hue is global, used by all arms & is incremented each time this function is called
 */
void color() {

    for(int currArm = 0; currArm < NUM_ARMS; currArm++)
    {
        if (arms[currArm].isActive)
        {
            twinkle(currArm);
        }
        else
        {
            rainbow(currArm);
        }

        arms[currArm].strip.show();
    }

    currRainbowHue += RAINBOW_HUE_INC_AMOUNT;
    if (currRainbowHue >= MAX_RAINBOW_HUE)
    {
        currRainbowHue = 0;
    }
} 

/**
 * Main loop
 */
void loop() {

  readEncoders();
  color();
}
