      //V0 works with only the pulsating rod
//V1 Added in t                                                                           `   he homing sequence to pulsating rod and puck incrementing as a sub-programs
//V2 Added in the homing sequence for the rotater, changed the acceleration for both homing sequences to be used
//V3 Finalized rotater code and calibrated it, need to add in calibration for the pulsating rod and serial command
//V4 Added in Serial incrementing, working

//pulsating rod Dir+ = pin D9, yellow
//pulsating rod Pul+ = pin D10, white
//pulsating rod limit = pin A4/D18, blue
//rotater Dir+ = pin D11, blue
//rotater Pul+ = pin D12, green
//rotater limit = pin A0/D14, blue

#include <AccelStepper.h>
#include <Adafruit_NeoPixel.h>

// Constants/variables related to Neopixel lighting
const int pixelCount = 50; 

    ; // 160 per meter
const int pixelPin = 5;
Adafruit_NeoPixel strip(pixelCount, pixelPin, NEO_GRB + NEO_KHZ800);
const int rainbowMode = 0;
const int whiteMode = 1;
const int busyMode = 2;
const int blinkMode = 3;
int currLightMode = rainbowMode; // Start rainbow
int currBrightness = 50;
long startTimeMs = 0;
long blinkWaitMs = 10000;
long globalHue = 0;

// Constants/variables related rod/puck movment & rotation
AccelStepper pulsatingRodStepper(1, 10, 9); // step pin = 10, dir pin = 9
AccelStepper rotaterStepper(1, 12, 11); // step pin = 12, dir pin = 11

int numberOfPucks = 48;
int numberOfPucksLeft = numberOfPucks;
int numberOfStepsPerPuck = 133;

int numberOfStepsToRotate = 8000;
int rotaterStartPosition = 2300;
long nextPosition = rotaterStartPosition + numberOfStepsToRotate;

int pulsatingRodLimitSwitchPin = 18;
int pulsatingRodStartPosition = 1250; // 850 start + 3 pucks worth of increments to 'new' start
int pulsatingRodStepperSpeed = 200;
int pulsatingRodStepperAccel = 500;
bool pulsatingRodLimitSwitchStatus = LOW;

int rotaterRodLimitSwitchPin = 14;
int rotaterStepperSpeed = 2000;
int rotaterStepperAccel = 300;
bool rotaterLimitSwitchStatus = LOW;

void runLightMode()
{
    if (currLightMode == rainbowMode)
    {
        // Idle rainbow
        for(int i = 0; i < strip.numPixels(); i++)
        { 
            int pixelHue = globalHue + (i * 65536L / strip.numPixels());
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }
        strip.show();
        delay(20);

        globalHue += 256;
        if(globalHue > 5 * 65536)
        {
            globalHue = 0;
        }
    }
    else if (currLightMode == whiteMode)
    {
        // Set our pixels to white for picture taking
        for(int i = 0; i < strip.numPixels(); i++)
        {
            strip.setPixelColor(i, 255, 255, 255);
        }
        strip.show(); 

        delay(30);
    }
    else if (currLightMode == busyMode)
    {
        uint8_t red = random(256);
        uint8_t green = random(256);
        uint8_t blue = random(256);

        // Toggle Random color for busy mode
        for(int i = 0; i < strip.numPixels(); i++)
        {
            strip.setPixelColor(i, strip.Color(red, green, blue));
            strip.show();
            delay(30);
            strip.setPixelColor(i, 0);
            strip.show();
        }  
    }
    else if (currLightMode == blinkMode)
    {
        strip.fill(0xFF0000);
        strip.show();
        delay(350);

        strip.fill(0x000000);
        strip.show();
        delay(350);

        // Check if we should transition from blink mode
        if ((startTimeMs + blinkWaitMs) < millis())
        {
            currLightMode = whiteMode;
        }
    }
}

void setup() 
{
    Serial.begin(9600);  // Initialize Serial Monitor

    // Light setup
    strip.begin();
    strip.show();
    strip.setBrightness(currBrightness);
    currLightMode = rainbowMode;

    // Rod/puck motor setup
    pinMode(pulsatingRodLimitSwitchPin, INPUT);
    pinMode(rotaterRodLimitSwitchPin, INPUT); 

    pulsatingRodStepper.setMaxSpeed(pulsatingRodStepperSpeed);       // movement speed
    pulsatingRodStepper.setSpeed(pulsatingRodStepperSpeed);           
    pulsatingRodStepper.setAcceleration(pulsatingRodStepperAccel);

    rotaterStepper.setMaxSpeed(rotaterStepperSpeed);       // movement speed
    rotaterStepper.setSpeed(rotaterStepperSpeed);           
    rotaterStepper.setAcceleration(rotaterStepperAccel);

    // On startup: re-home then move to 'first' puck location
    homingSequencePulsatingRod();
    homingSequenceRotater();
    rotaterStepper.move(-rotaterStartPosition); // to make it go backwards
    while (rotaterStepper.distanceToGo() != 0)
    {
        rotaterStepper.run();
    }
    rotaterStepper.stop();

    Serial.println("Starting!");
}

void loop()
{
    puckIncrement();
    homingSequencePulsatingRod();
    rotaterIncrement();
}

// Reset the pulsating rod to 0 at the limit switch
void homingSequencePulsatingRod()
{
    pulsatingRodLimitSwitchStatus = digitalRead(pulsatingRodLimitSwitchPin);
    if (pulsatingRodLimitSwitchStatus == LOW)
    {
        pulsatingRodStepper.move(-100);
        while (pulsatingRodStepper.distanceToGo() != 0)
        {
            pulsatingRodStepper.run();
        }
    }

    pulsatingRodLimitSwitchStatus = digitalRead(pulsatingRodLimitSwitchPin);
    
    while (pulsatingRodLimitSwitchStatus == HIGH)
    {
        pulsatingRodLimitSwitchStatus = digitalRead(pulsatingRodLimitSwitchPin);
        pulsatingRodStepper.move(20);
        pulsatingRodStepper.run();
    }

    pulsatingRodStepper.stop();
    pulsatingRodStepper.setCurrentPosition(0);
}

void puckIncrement()
{
    pulsatingRodStepper.move(-pulsatingRodStartPosition);

    while (pulsatingRodStepper.distanceToGo() != 0) 
    {
        pulsatingRodStepper.run();
    }

    /**
     * This is Anil's main loop - we sit here waiting for certain character(s) from the PC
     * While waiting for a puck increment signal, we need to also handle lighting
     * This means we need to be able to read characters from the PC and translate:
     *  'I' - increment our puck, possibly causing us to rotate/re-home (also transition back to rainbow/idle lighting)
     *  'P' - Picture - blink for a few seconds then full brightness
     *  'B' - busy mode, yellow chasing until enraving is finished (then puck increment - back to idle)
     */
    while (numberOfPucksLeft > 0) {

        if (Serial.available() > 0)
        {
            // Got input
            char incomingChar = Serial.read();

            Serial.println("Got input!");
            Serial.println(incomingChar);
            
            // if 'P', picture mode
            if (incomingChar == 'P')
            {
                // Our first time in - record the time
                if (currLightMode == rainbowMode)
                {
                    startTimeMs = millis();
                    
                    // Full bright until we go back to idle/rainbow
                    currBrightness = 255;
                    strip.setBrightness(currBrightness);
                    strip.show();
                    currLightMode = blinkMode;
                }
                else if ((startTimeMs + blinkWaitMs) < millis())
                {
                    currLightMode = whiteMode;
                }
            }

            // else if 'B' busy mode
            else if (incomingChar == 'B')
            {
                currLightMode = busyMode;
            }

            // else if 'I' puck increment/possible rotate - stay busy
            else if (incomingChar == 'I')
            {
                // Move for one puck
                pulsatingRodStepper.move(-numberOfStepsPerPuck);
                while (pulsatingRodStepper.distanceToGo() != 0)
                {
                    pulsatingRodStepper.run();
                }

                // Decrement the # of pucks, might kick us out of the loop!
                numberOfPucksLeft = numberOfPucksLeft - 1;
                delay(1000);
            }

            else if (incomingChar == 'D')
            {
                // Change back to rainbow/idle & decrease brightness
                currBrightness = 50;
                strip.setBrightness(currBrightness);
                strip.show();
                currLightMode = rainbowMode;
            }
        }
        else
        {
            // Run the lights one time each loop
            runLightMode();
        }
    }

    numberOfPucksLeft = numberOfPucks;

}

// Reset the rotater  to 0 at the limit switch
void homingSequenceRotater()
{
   
    rotaterLimitSwitchStatus = digitalRead(rotaterRodLimitSwitchPin);
    if (rotaterLimitSwitchStatus == LOW)
    {
        rotaterStepper.move(-1000);
        while (rotaterStepper.distanceToGo() != 0) 
        {
            rotaterStepper.run();
        }
    }

    rotaterLimitSwitchStatus = digitalRead(rotaterRodLimitSwitchPin);
    
    while (rotaterLimitSwitchStatus == HIGH)
    {
        rotaterLimitSwitchStatus = digitalRead(rotaterRodLimitSwitchPin);
        rotaterStepper.move(100);
        rotaterStepper.run();
    }

    rotaterStepper.stop();
    rotaterStepper.setCurrentPosition(0);
    delay(1000);
}

// Once the stack is empty then rotate to the next stack
void rotaterIncrement()
{
    rotaterStepper.moveTo(-nextPosition); //to make it go backwards
    while (rotaterStepper.distanceToGo() != 0)
    {
        rotaterStepper.run();                          // accelerates + moves
    }
    
    nextPosition = numberOfStepsToRotate + nextPosition;
    
    //so we do not hit the limit screw and to re-home in case we lost any steps
    if (nextPosition > 80000)
    {
        nextPosition = rotaterStartPosition;
        puckIncrement();//needed to use the last position
        homingSequencePulsatingRod();//
        rotaterStepper.moveTo(-800); //to make it go almost the start
        
        while (rotaterStepper.distanceToGo() != 0)
        {
            rotaterStepper.run();                          
        }
        homingSequenceRotater();
        rotaterStepper.move(-rotaterStartPosition);//to make it go backwards
        
        while (rotaterStepper.distanceToGo() != 0)
        {
            rotaterStepper.run();
        }
        
        rotaterStepper.stop();
        nextPosition = numberOfStepsToRotate + nextPosition;
    }
    delay(500);                                     // optional delay between moves
}
