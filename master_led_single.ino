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
const int timeBetweenChecksMs = 1000; //200;
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
int stripOneEncoderPos = 0;
long lastChangeOne = 0;
const int STRIP_ONE_DATA_PIN = 12;
const int NUM_LEDS_IN_STRIP = 700;
Adafruit_NeoPixel stripOne = Adafruit_NeoPixel(NUM_LEDS_IN_STRIP, STRIP_ONE_DATA_PIN, NEO_GRB + NEO_KHZ800);

// Coloring variables/constants
const int colorArray[] = {0xFF6633, 0xFFB399, 0xFF33FF, 0xFFFF99, 0x00B3E6, 
		  0xE6B333, 0x3366E6, 0x999966, 0x99FF99, 0xB34D4D,
		  0x80B300, 0x809900, 0xE6B3B3, 0x6680B3, 0x66991A, 
		  0xFF99E6, 0xCCFF1A, 0xFF1A66, 0xE6331A, 0x33FFCC,
		  0x66994D, 0xB366CC, 0x4D8000, 0xB33300, 0xCC80CC, 
		  0x66664D, 0x991AFF, 0xE666FF, 0x4DB3FF, 0x1AB399,
		  0xE666B3, 0x33991A, 0xCC9999, 0xB3B31A, 0x00E680, 
		  0x4D8066, 0x809980, 0xE6FF80, 0x1AFF33, 0x999933,
		  0xFF3380, 0xCCCC00, 0x66E64D, 0x4D80CC, 0x9900B3, 
		  0xE64D66, 0x4DB380, 0xFF4D4D, 0x99E6E6, 0x6666FF};
const int timeToRainbowMs = 10000;
long currRainbowHue  = 0;

void setup()
{
    Serial.begin(9600);
  
    // Setup our RS485 control - putting it into receive mode to start
    pinMode(SSERIAL_CTRL_PIN, OUTPUT);    
    digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);

    // Setup our NeoPixel strip(s)
    stripOne.begin();
    stripOne.setBrightness(25);
    stripOne.rainbow(colorArray[0]);
    stripOne.show();

    // Setup our TX/RX communication
    Serial1.begin(115200);

    // Delay a small amount of time - so that our slaves can initialize
    delay(MASTER_DELAY_MS);

    Serial.println("Master Ready");
}

void read_encoder_value(int encoderIdx)
{
	Serial.println("Attempting to talk to encoder ID:");
	Serial.println(ENCODER_IDS[encoderIdx]);

    // Attempt to talk to the given encoder - via encoderId
    digitalWrite(SSERIAL_CTRL_PIN, RS485_TRANSMIT);
    Serial1.write(ENCODER_IDS[encoderIdx]);
    delay(1);
    digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);

    // Wait for a response
    long startTime = millis();
    while(!Serial1.available())
    {
        delay(10);

        // This encoder failed to respond - mark it as missing & bounce out
        if(startTime + timeToWaitMs < millis())
        {
        	Serial.println("Failed to communicate with encoder - marking as absent");
        	encoderExists[encoderIdx] = false;
        	break;
        }
    }

    // If the encoder responded - record its position
    if(encoderExists[encoderIdx])
    {
	    byteReceived = Serial1.read();
	    delay(10);
	    Serial.println("Read Value:");
	    Serial.println(byteReceived);
	    if (encoderPosition[encoderIdx] != byteReceived)
	    {
	        encoderPosition[encoderIdx] = byteReceived;
	        lastChanged[encoderIdx] = millis();
	    }
	}

	delay(10);
}

void color_leds(int encoderIdx)
{
	if(encoderExists[encoderIdx])
	{
	    // Color our LED strips based on associated slaves last read encoder position
	    if (lastChanged[encoderIdx] + timeToRainbowMs < millis())
	    {
	        stripOne.rainbow(currRainbowHue);
	    }
	    else
	    {
	        stripOne.fill(colorArray[encoderPosition[encoderIdx]], 0, NUM_LEDS_IN_STRIP - 1);
	    }
	    stripOne.show();
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
    // if (lastChangeOne + timeToRainbowMs < millis())
    // {
    //     stripOne.rainbow(currRainbowHue);
    // }
    // else
    // {
    //     stripOne.fill(colorArray[stripOneEncoderPos], 0, NUM_LEDS_IN_STRIP - 1);
    // }
    // stripOne.show();
	for(int encoderIdx = 0; encoderIdx < NUM_ENCODERS; encoderIdx++)
	{
		color_leds(encoderIdx);
	}

    // Update global rainbow hue value
    currRainbowHue += 128;
    if (currRainbowHue >= 327680)
    {
        currRainbowHue = 0;
    }

}