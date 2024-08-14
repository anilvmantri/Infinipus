/**
 * Slave Encoder Software. Uses an Adafruit Stemma QT breakout board with an attached rotary encoder, 
 * as well as an RS485 module to communicate over UART (serial) with a master controller. Basic loop:
 * - Read current encoder value
 * - Check if we have serial data ready - if not continue looping
 * - If we have data ready, check to see if it's addressed to us - ie byteReceived == encoderId
 * - If it is addressed to us, setup our RS485 module to transmit
 * - Send our current encoder value (encoderPosition) & set ourselves back to receive
 * - Continue looping
 */

#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

// Slave variables/constants
const int SLAVE_DELAY_MS = 2000;
const int NUM_ENCODER_POSITIONS = 16;

// RS485 variables/constants
const int SSERIAL_CTRL_PIN = 3;
const int RS485_TRANSMIT = HIGH;
const int RS485_RECEIVE = LOW;
int byteReceived;

// Encoder variables/constants
const int SS_SWITCH = 24;
const int SS_NEOPIX = 6;
const int NUM_ENCODERS = 8;
const int SEESAW_ADDRS[NUM_ENCODERS] = {0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D};
const char ENCODER_IDS[NUM_ENCODERS] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
int encoderPosition = 0;
char encoderId = "";
bool encoderFound = false;
Adafruit_seesaw ss;
seesaw_NeoPixel sspixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);

void setup() 
{
	Serial.begin(9600);

    // Find which encoder is connected - if any
    for (uint32_t encoderAddrIdx = 0; encoderAddrIdx < NUM_ENCODERS; encoderAddrIdx++)
    {
	    if (ss.begin(SEESAW_ADDRS[encoderAddrIdx]) && sspixel.begin(SEESAW_ADDRS[encoderAddrIdx]))
	    {
	    	encoderFound = true;
	    	encoderId = ENCODER_IDS[encoderAddrIdx];
	        Serial.println("Found Encoder, Addr (hex) - ID (char)");
	        Serial.println(SEESAW_ADDRS[encoderAddrIdx], HEX);
	        Serial.println(encoderId);
	        break;
    	}
    	delay(10);
    }

	// Setup the Encoder - if we were able to find one connected
    if(encoderFound)
    {
	    sspixel.setBrightness(20);
	    sspixel.show();
	    ss.pinMode(SS_SWITCH, INPUT_PULLUP);
	    ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
	    ss.enableEncoderInterrupt();
	    encoderPosition = ss.getEncoderPosition();
	}
	else
	{
		Serial.println("Failed to find encoder");
	}

    // Setup our RS485 control - putting it into receive mode to start
    pinMode(SSERIAL_CTRL_PIN, OUTPUT);  
    delay(10);
    digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);  // Put RS485 in receive mode

    // Setup our TX/RX communication
    Serial1.begin(115200);


    // Delay a small amount of time - so that the master can initialize
    delay(SLAVE_DELAY_MS);

    Serial.println("Slave Ready");
}

void loop() 
{
	if(encoderFound)
	{
    	encoderPosition = ss.getEncoderPosition();
    }

    // Check if there is new data on the bus
    if (Serial1.available())
    {
        byteReceived = Serial1.read();
        delay(10);              
            
        // Check if the master is asking us for our current encoder value
        if (byteReceived == encoderId)
        {
            digitalWrite(SSERIAL_CTRL_PIN, RS485_TRANSMIT);

            // Make sure our encoder value maps in the range 0 -> 15 before sending back our data
            encoderPosition = encoderPosition % NUM_ENCODER_POSITIONS;
            if (encoderPosition < 0)
            {
                encoderPosition += NUM_ENCODER_POSITIONS;
            }
            Serial1.write(encoderPosition);
            Serial1.flush();
            delay(10);
            digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);
        }
    }  
}