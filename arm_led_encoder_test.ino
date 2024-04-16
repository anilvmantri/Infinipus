/**
 * Test code - loops through all possible outputLedPinMap output pins, delaying
 * five seconds/looping the following pattern:
 * 
 * rainbow -> delay -> color/compColor -> delay -> etc
 * 
 * See arm_led_encoder.ino for a more detailed explanation
 */

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

const int NUM_ARMS = 7;                       // Number of arms
const int LED_COUNT = 350;                    // Number of NeoPixels attached to each arm
const int NUM_COLOR_MAPPINGS = 15;            // Number of colors
const char DEFAULT_BRIGHTNESS = 50;           // Default value of brightness, max is 255
const long MAX_RAINBOW_HUE = 327680;          // Maximum rainbow hue value
const long RAINBOW_HUE_INC_AMOUNT = 256;      // Amount to increment our global rainbow hue value by each coloring loop
const long TEST_DELAY_MS = 2000;              // Amount of time to delay between each test sequence

/**
 * Structure containing an individual arms information
 */
typedef struct
{
    Adafruit_NeoPixel strip; // NeoPixel strip object for this arm
    int currentColorIndex;   // The current color index, based on last input/encoder reading
    bool isActive;           // Flag to determine if we're currently rainbowing or not
    long activedTimeMs;      // Last time in miliseconds this arm updated with a new non-rainbow color
    bool swap;               // Variable allowing us to 'twinkle' between each color/comp color
} arm_t;

// Array of all arms covered by this arduino
arm_t arms[NUM_ARMS];

// Array mapping arms -> output pins to LED strip
int outputLedPinMap[NUM_ARMS] = {8, 7, 6, 5, 4, 3, 2};

// Array mapping encoder index -> color/complementary colors
int colors[NUM_COLOR_MAPPINGS] = {0x12B8FF, 0x01DC03, 0xFFE62D, 0xFD4499, 0xDF19FB, 0x5E57FF, 0xF23CA6, 0xFF9535, 0x4BFF36, 0x02FEE4, 0xF500EB, 0x0CD4FF, 0x8DFF0A, 0xFFEF06, 0xFF3A06};
int compColors[NUM_COLOR_MAPPINGS] = {0xff5912, 0xdc01da, 0x2d46ff, 0x44fda8, 0x35fb19, 0xf8ff57, 0x3cf288, 0x359fff, 0xea36ff, 0xfe021c, 0x00f50a, 0xff370c, 0x7c0aff, 0x0616ff, 0x06cbff};

long currRainbowHue = 0;  // Global variable for the current rainbow hue to use for all arms rainbowing
long startTimeMs = 0;     // Start time for the current test sequence
int currSeq      = 0; // Current sequence to test

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

    for (int pixel = 0; pixel < arms[currArm].strip.numPixels(); pixel++) {
        if (pixel % 2 == 0)
        {
            if (arms[currArm].swap)
            {
                arms[currArm].strip.setPixelColor(pixel, compColors[currSeq]);
            }
            else
            {
                arms[currArm].strip.setPixelColor(pixel, colors[currSeq]);
            }
        }
        else
        {
            if (arms[currArm].swap)
            {
                arms[currArm].strip.setPixelColor(pixel, colors[currSeq]);
            }
            else
            {
                arms[currArm].strip.setPixelColor(pixel, compColors[currSeq]);  
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
void testColor() {

    for(int currArm = 0; currArm < NUM_ARMS; currArm++)
    {
        if (currSeq < NUM_COLOR_MAPPINGS)
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

    if (millis() > startTimeMs + TEST_DELAY_MS)
    {
        startTimeMs = millis();
        currSeq += 1;
        if (currSeq > NUM_COLOR_MAPPINGS)
        {
            currSeq = 0;
        }
    }

    testColor();
}
