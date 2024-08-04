/**
 * Slave Encoder Software. Uses an Adafruit Stemma QT breakout board with an attached rotary encoder, 
 * as well as an RS485 module to communicate over UART (serial) with a master controller. Basic loop:
 * - Read current encoder value
 * - Check if we have serial data ready - if not continue looping
 * - If we have data ready, check to see if it's addressed to us - ie byteReceived == SLAVE_ENCODER_ID
 * - If it is addressed to us, setup our RS485 module to transmit
 * - Send our current encoder value (encoderPosition) & set ourselves back to receive
 * - Continue looping
 */

#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

// Slave variables/constants
const int SLAVE_DELAY_MS = 2000;
const int SLAVE_ENCODER_ID = 'A';
const int NUM_ENCODER_POSITIONS = 16;

// RS485 variables/constants
const int SSERIAL_CTRL_PIN = 3;
const int RS485_TRANSMIT = HIGH;
const int RS485_RECEIVE = LOW;
int byteReceived;

// Encoder variables/constants
const int SS_SWITCH = 24;
const int SS_NEOPIX = 6;
const int SEESAW_ADDR = 0x39;
int encoderPosition;
Adafruit_seesaw ss;
seesaw_NeoPixel sspixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);

void setup() 
{
    // Check to make sure the encoder exists
    if (! ss.begin(SEESAW_ADDR) || ! sspixel.begin(SEESAW_ADDR)) {
        Serial.println("Couldn't find seesaw on default address");
        while(1) delay(10);
    }

    // Setup the Encoder - for some reason we also need to setup both the onboard pixel & the interrupt (unused)
    sspixel.setBrightness(20);
    sspixel.show();
    ss.pinMode(SS_SWITCH, INPUT_PULLUP);
    ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
    ss.enableEncoderInterrupt();
    encoderPosition = ss.getEncoderPosition();

    // Setup our RS485 control - putting it into receive mode to start
    pinMode(SSERIAL_CTRL_PIN, OUTPUT);  
    delay(10);
    digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);  // Put RS485 in receive mode

    // Setup our TX/RX communication
    Serial1.begin(115200);

    // Delay a small amount of time - so that the master can initialize
    delay(SLAVE_DELAY_MS);
}

void loop() 
{
    encoderPosition = ss.getEncoderPosition();

    // Check if there is new data on the bus
    if (Serial1.available())
    {
        byteReceived = Serial1.read();
        delay(10);              
            
        // Check if the master is asking us for our current encoder value
        if (byteReceived == SLAVE_ENCODER_ID)
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