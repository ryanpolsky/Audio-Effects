
/*
 ____  _____ _        _
 | __ )| ____| |      / \
 |  _ \|  _| | |     / _ \
 | |_) | |___| |___ / ___ \
 |____/|_____|_____/_/   \_\
 
 Filter Effect
 Ryan Polsky (rap5vj), MUSI 4545, University of Virginia, October 9th 2017

 The effect has five user inputs. There is a switch to turn the filter on or off. There are 3 pots that control the gain/cut of the three filters, and a fourth pot to control the
 frequency of the peak notch

 1) The Low Shelf gain/cut pot ranges from -20 dB to 20dB, with the Low Shelf frequency always 700Hz
 2) The High Shelf gain/cut pot ranges from -20 dB to 20dB with the High Shelf frequency always 1500Hz
 3) The Peak/Notch gain/cut pot ranges from -20 dB to 20dB
 4) The Peak/Notch frequency ranges from 27 to 123 in MIDI Note Number
 
 */


// Include any other files or libraries we need -- -- -- -- -- -- -- -- --
#include <Bela.h>
#include <stdlib.h>
#include <BiQuad.h>
#include <Mu45FilterCalc.h>

// Defines -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
#define FCMIN_NN 27.0
#define FCMAX_NN 123.0
#define FILTQ 3.0
#define LS_NN 27
#define HS_NN 123
#define LS_HZ 700
#define HS_HZ 1500
#define MIN_DB -20
#define MAX_DB 20

// Declare global variables   -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
int gAudioFramesPerAnalogFrame;
stk::BiQuad lowShelfL, lowShelfR, highShelfL, highShelfR, peakNotchL, peakNotchR;

// Global variables for debugging
int gPCount = 0;                // counter for printing
float gPrintInterval = 0.5;        // how often to print, in seconds

// Convert MIDI note-number to Hz   -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
float NN2HZ(float NN){
    return 440.0*powf(2, (NN-69.0)/12.0);
}

// Setup and initialize stuff -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
bool setup(BelaContext *context, void *userData)
{
    gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
    
    // Set the digital pins to input or output
    pinMode(context, 0, P8_08, INPUT);  //Switch
    pinMode(context, 0, P8_07, OUTPUT);  //LED
    
    return true;
}


// Render each block of input data  -- -- -- -- -- -- -- -- -- -- -- -- --
void render(BelaContext *context, void *userData)
{
    // Declare local variables
    float out_l, out_r;
    float ls_gain, hs_gain, pn_gain, pn_fc;
    
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
                
                // Pot 1: Control the gain/cut for Low-Shelf Filter
                ls_gain = analogRead(context, n/gAudioFramesPerAnalogFrame,0); // read analog pin 0
                ls_gain = map(ls_gain, 0.0001, 0.827, MIN_DB, MAX_DB);
                
                // Pot 2: Control the gain/cut for High-Shelf Filter
                hs_gain = analogRead(context, n/gAudioFramesPerAnalogFrame, 0); // read analog pin 2
                hs_gain = map(hs_gain, 0.0001, 0.827, MIN_DB, MAX_DB);
                
                // Pot 3: Control the gain/cut for Peak/Notch Filter
                pn_gain = analogRead(context, n/gAudioFramesPerAnalogFrame, 1); // read analog pin 2
                pn_gain = map(pn_gain, 0.0001, 0.827, MIN_DB, MAX_DB);
                
                // Pot 4: Control the cutoff frequency for the Peak/Notch Filter
                pn_fc = analogRead(context, n/gAudioFramesPerAnalogFrame, 2); // read analog pin 3
                pn_fc = map(pn_fc, 0.0001, 0.827, LS_NN, HS_NN);  //pn_fc ranges between ls_fc and hs_fc
                pn_fc = NN2HZ(pn_fc);
                
                
                // calculate and set the coefficients for the low-shelf filter
                
                float lsCoeffs[5];    // an array of 5 floats for filter coeffs
                Mu45FilterCalc::calcCoeffsLowShelf(lsCoeffs, HS_HZ, ls_gain, context->audioSampleRate);
                lowShelfL.setCoefficients(lsCoeffs[0], lsCoeffs[1], lsCoeffs[2], lsCoeffs[3], lsCoeffs[4]);
                lowShelfR.setCoefficients(lsCoeffs[0], lsCoeffs[1], lsCoeffs[2], lsCoeffs[3], lsCoeffs[4]);
                
                // calculate and set the coefficients for the high-shelf filter
                
                float hsCoeffs[5];    // an array of 5 floats for filter coeffs
                Mu45FilterCalc::calcCoeffsHighShelf(hsCoeffs, LS_HZ , hs_gain, context->audioSampleRate);
                highShelfL.setCoefficients(hsCoeffs[0], hsCoeffs[1], hsCoeffs[2], hsCoeffs[3], hsCoeffs[4]);
                highShelfR.setCoefficients(hsCoeffs[0], hsCoeffs[1], hsCoeffs[2], hsCoeffs[3], hsCoeffs[4]);
                
                // calculate and set the coefficients for the peak/notch filter
                float pnCoeffs[5];    // an array of 5 floats for filter coeffs
                Mu45FilterCalc::calcCoeffsPeak(pnCoeffs, pn_fc, pn_gain, FILTQ, context->audioSampleRate);
                peakNotchL.setCoefficients(pnCoeffs[0], pnCoeffs[1], pnCoeffs[2], pnCoeffs[3], pnCoeffs[4]);
                peakNotchR.setCoefficients(pnCoeffs[0], pnCoeffs[1], pnCoeffs[2], pnCoeffs[3], pnCoeffs[4]);
                
                if(gPCount++ % (int)(context->audioSampleRate*gPrintInterval/gAudioFramesPerAnalogFrame) == 0)
                {
//                    printf("Fc: %f\n", pn_fc);
//                    printf("PN Gain: %f\n", pn_gain);
//                    printf("HS Gain: %f\n", hs_gain);
//                    printf("LS Gain: %f\n", ls_gain);
                }
            }
            
            // Audio rate stuff here...
            // Get input audio samples
            out_l = audioRead(context,n,0);
            out_r = audioRead(context,n,1);
            
            // Process the audio
            out_l = lowShelfL.tick(out_l);
            out_l = peakNotchL.tick(out_l);
            out_l = highShelfL.tick(out_l);
            
            out_r = lowShelfR.tick(out_r);
            out_r = peakNotchR.tick(out_r);
            out_r = highShelfR.tick(out_r);
            
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

