
/*
 ____  _____ _        _
 | __ )| ____| |      / \
 |  _ \|  _| | |     / _ \
 | |_) | |___| |___ / ___ \
 |____/|_____|_____/_/   \_\
 
 Ping Pong Delay Audio Effect
 Ryan Polsky (rap5vj), Musi 4545, University of Virginia, October 20th 2017
 
 The effect I designed has four user inputs. There is a switch to turn the delay on or off. In addition, there are 3 pots that control certain aspects of the delay
 
 1) The delay time pot controls the frequency of the delays and ranges from 50ms to 500ms
 2) The wet/dry mix pot controls the relative presence of the echoes and ranges from 0 to 1, where 0 is only dry signal and 1 is only wet signal
 3) The feedback pot controls how quickly the echoes fade away and ranges from 0 to 0.8. I capped the range at 0.8 so the effect would not echo forever.

 
 */


// Include any other files or libraries we need -- -- -- -- -- -- -- -- --
#include <Bela.h>
#include <stdlib.h>
#include "Delay.h"

// Defines -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
#define MIN_DLY_MS 50
#define MAX_DLY_MS 500
#define MIN_DB -20
#define MAX_DB 6

// Declare global variables   -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
int gAudioFramesPerAnalogFrame;
stk::Delay delayL, delayR;

int calcSampsFromMsec(float msec, float sampRate){
    float sec = msec * 0.001;  // Convert from ms to s
    int samps = sampRate * sec; // Multiply seconds by samples/seconds to get samples
    return samps;
}

// Global variables for debugging
int gPCount = 0;                // counter for printing
float gPrintInterval = 0.5;        // how often to print, in seconds
float lastDelayTimeMsec = 100;


// Setup and initialize stuff -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
bool setup(BelaContext *context, void *userData)
{
    gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
    
    // Set the digital pins to input or output
    pinMode(context, 0, P8_08, INPUT);  //Switch
    pinMode(context, 0, P8_07, OUTPUT);  //LED
    
    // Set max delay
    int maxSamps = calcSampsFromMsec(MAX_DLY_MS,context->audioSampleRate);
    delayL.setMaximumDelay(maxSamps);
    delayR.setMaximumDelay(maxSamps);
    delayL.setDelay(20000);
    delayR.setDelay(20000);
    
    return true;
}


// Render each block of input data  -- -- -- -- -- -- -- -- -- -- -- -- --
void render(BelaContext *context, void *userData)
{
    // Declare local variables
    float out_l, out_r;
    float delayTime, wetGain, dryGain, pan, panL, panR, fb;
    
    // These calculations we can do once per buffer
    int lastDigFrame = context->digitalFrames - 1;                    // use the last digital frame
    int switchStatus = digitalRead(context, lastDigFrame, P8_08);      // read the value of the switch
    digitalWriteOnce(context, lastDigFrame, P8_07, switchStatus);     // write the status to the LED
    
    // Use the switchStatus to select between 2 different audio processing algorithms.
    if(switchStatus)
    {
        // WHEN SWITCH IS ON: process the audio through a filter with logarithmic frequency control
        for(unsigned int n = 0; n < context->audioFrames; n++)
        {
            if(!(n % gAudioFramesPerAnalogFrame))
            {
                
                // Pot 1: Control the delay time
                delayTime = analogRead(context, n/gAudioFramesPerAnalogFrame, 0);
                delayTime = map(delayTime, 0.0001, 0.827, MIN_DLY_MS, MAX_DLY_MS);
                delayTime = (delayTime > MAX_DLY_MS) ? MAX_DLY_MS: delayTime;
                
                //Change the delay time only when the potentiometer voltage changes significantly
                float DELAY_STEP = 5;
                // // try different values here
                if(fabs(delayTime-lastDelayTimeMsec) >= DELAY_STEP){
                    lastDelayTimeMsec = delayTime;
                    float targetDelaySamps = calcSampsFromMsec(delayTime, context->audioSampleRate);
                    delayL.setDelay(targetDelaySamps);
                    delayR.setDelay(targetDelaySamps);
                }
                
                
                // Pot 2: Control the wet/dry mix
                wetGain = analogRead(context, n/gAudioFramesPerAnalogFrame, 1); // read analog pin 1
                wetGain = map(wetGain, 0.0001, 0.827, 0, 1);
                dryGain = 1 - wetGain;
                
                // Pot 3: Control depth of the pan
                fb = analogRead(context, n/gAudioFramesPerAnalogFrame, 2); // read analog pin 2
                fb = map(fb, 0.0001, 0.827, 0, 0.8);
                
                
                if(gPCount++ % (int)(context->audioSampleRate*gPrintInterval/gAudioFramesPerAnalogFrame) == 0)
                {
                    printf("Time: %f\n", delayTime);
                    printf("Wet: %f\n", wetGain);
                    printf("FB: %f\n", fb);
                    // printf("LS Gain: %f\n", ls_gain);
                }
            }
            
            // Audio rate stuff here...
            // Get input audio samples
            out_l = audioRead(context,n,0);
            out_r = audioRead(context,n,1);
            
            float dryL, dryR, wetL, wetR, tempL, tempR;
            
            dryL = out_l * dryGain;
            dryR = out_r * dryGain;
            tempL = out_l + delayR.nextOut() * fb;
            tempR = out_r + delayL.nextOut() * fb;
            wetL = delayL.tick(tempL) * wetGain;
            wetR = delayR.tick(tempR) * wetGain;
            out_l = dryL + wetL;
            out_r = dryR + wetR;
            
            
            // Write output audio samples
            audioWrite(context, n, 0, out_l);
            audioWrite(context, n, 1, out_r);
        }
    }
    else
    {
        // WHEN SWITCH IS OFF: pass audio through with no changes
        for(unsigned int n = 0; n < context->audioFrames; n++)
        {
            
            
            // Get input audio samples
            out_l = audioRead(context,n,0);
            out_r = audioRead(context,n,1);
            
            // Write output audio samples
            audioWrite(context, n, 0, out_l);
            audioWrite(context, n, 1, out_r);
        }
    }
}


void cleanup(BelaContext *context, void *userData)
{
    // Nothing to do here
}


