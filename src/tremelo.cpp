/*
 ____  _____ _        _
 | __ )| ____| |      / \
 |  _ \|  _| | |     / _ \
 | |_) | |___| |___ / ___ \
 |____/|_____|_____/_/   \_\
 
Tremelo and Pan Audio Effect
Ryan Polsky (rap5vj), MUSI 4545, University of Virginia, October 9th 2017
 
This effect I designed has three user inputs. The first knob controls the overall pan, the other two knobs
control tremolo depth and tremolo frequency.
1) The pan knob ranges from 0.0 to 1.0, where 0 is panned all the way left, 1 is panned is the way right, and 0.5 is panned center
2) The tremolo frequency knob affects how fast the oscillation is. The knob goes from 0 Hz to 10 Hz, so all the way left means no oscillation and all the right is10 Hz. This was chosen so the sound would not distort when the knob is turned all the way to the right
3) The tremolo depth controls how much the sound actually oscillates by. It ranges from 0dB to -26dB where all the way left at 0dB means sound always comes though at 6dB. All the way left, the sounds oscillates between 6dB and -20dB.
 
 */


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// Include any files or libraries we need
#include <Bela.h>
#include <cmath>
#include "Mu45LFO.h"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// Defines (useful for design choices and magic numbers)
#define PI 3.14159265359
#define MIN_DB -20.0
#define MAX_DB 6.0
#define MAX_FREQ 10.0

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// Declare any global variables

// Set the analog input channels to read from
int gSensorInputTremeloDepth = 0;		// analog in channel for tremelo depth
int gSensorInputTremeloFreq= 1;		// analog in channel for tremelo frequency
int gSensorInputPan = 2;		// analog in channel for pan potentiometer

// Global variables for audio processing and parameters
int gAudioFramesPerAnalogFrame;
float tremeloDepth = 1.0;
float tremeloFreq = 1.0;
const float dbMultiplier = MIN_DB - MAX_DB;
Mu45LFO lfo;

// Global variables for debugging
int gPCount = 0;				// counter for printing
float gPrintInterval = 0.5;		// how often to print, in seconds

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// This function gets run before anything else happens
bool setup(BelaContext *context, void *userData)
{
    
    // Check if analog channels are enabled
    if(context->analogFrames == 0 || context->analogFrames > context->audioFrames)
    {
        rt_printf("Error: this example needs analog enabled, with 4 or 8 channels\n");
        return false;
    }
    
    // Check that we have the right number of inputs and outputs.
    if(context->audioInChannels != context->audioOutChannels ||
       context->analogInChannels != context-> analogOutChannels){
        printf("Error: for this project, you need the same number of input and output channels.\n");
        return false;
    }
    
// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
    gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
    
    return true;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// This function gets called for each block of input data
void render(BelaContext *context, void *userData)
{
    // Declare any local variables you need
    float out_l = 0;
    float out_r = 0;
    float LFOout = 0;
    float gainDB = 0;
    float gainLin = 0;
    float pan = 0.0;
    float panGainL = 0.0;
    float panGainR = 0.0;
    
    // Step through each frame in the audio buffer
    for(unsigned int n = 0; n < context->audioFrames; n++)
    {
        // --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
        // Read the data from analog sensors, and calculate any alg params
        if(!(n % gAudioFramesPerAnalogFrame))
        {
            // tremeloDepth is the amount of volume fluctuation
            tremeloDepth = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInputTremeloDepth);
            tremeloDepth = map(tremeloDepth, 0.0005, 0.8345, dbMultiplier, 0.0 );
            
            
            // tremeloFreq is the frequency of volume fluctuation
            tremeloFreq = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInputTremeloFreq);
            tremeloFreq = map(tremeloFreq, 0.00009, 0.833, 0.0, MAX_FREQ);
            lfo.setFreq(tremeloFreq, context->audioSampleRate);
            
            // Pan parameter
            pan = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInputPan);
            pan = map(pan, 0.00009, 0.833, 0.0, 1.0 );
            panGainL = cos(pan * (PI/2.0));
            panGainR = sin(pan * (PI/2.0));
            
            // Tick the LFO and use the output to calculate the gain
            LFOout = lfo.tick();
            //if termelo depth is 0, gain is always 6db
            //the closer it gets to 1, gain alternates between 6db and -20db depedning on LFO
            gainDB = tremeloDepth*0.5*(1+LFOout) + 6.0;
            gainLin = powf(10.0, gainDB/20.0);
            
            
            // Print gain value (for debugging)
            if(gPCount++ % (int)(context->audioSampleRate*gPrintInterval/gAudioFramesPerAnalogFrame) == 0)
            {
//                rt_printf("Gain: %f\n", gainLin);
//                rt_printf("Freq: %f\n", tremeloFreq);
//                rt_printf("LFO: %f\n", LFOout);
//                rt_printf("Pan Right: %f\n", panGainR);
//                rt_printf("Pan Left: %f\n", panGainL);
                
            }
        }
        
        // --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
        // Read audio inputs from the input buffer
        out_l = audioRead(context,n,0);
        out_r = audioRead(context,n,1);
        
       	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
        // Process each audio sample (in this case apply the gain)
        out_l = gainLin * panGainL * out_l;
        out_r = gainLin * panGainR * out_r;
        
        
        
        // --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 
        // Write the samples into the output buffer -- done!
        audioWrite(context, n, 0, out_l);
        audioWrite(context, n, 1, out_r);
    }
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// This function gets called when exiting the program.
void cleanup(BelaContext *context, void *userData)
{
    
}
