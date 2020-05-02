#include <Bela.h>
#include <stdlib.h>
#include <libraries/Scope/Scope.h>
#include <libraries/PulseIn/PulseIn.h>
#include <cmath>
#include <algorithm>

PulseIn pulseInDown;
PulseIn pulseInUp;
Scope scope;
int gTriggerInterval = 2646; // how often to send out a trigger. 2646 samples are 60ms 
int gMinPulseLength = 7; //to avoid spurious readings
float gRescale = 58; // taken from the datasheet

int yinyang = 0; // I used this variable so that every 62 ms we fire one sensor alternately

//Sensor Down
unsigned int gTriggerAnalogOutChannelDown = 0; //channel to be connected to the module's TRIGGER pin - check the pin diagram in the IDE
unsigned int gEchoDigitalInPinDown = 1; //channel to be connected to the modules's ECHO pin (digital pin 1) - check the pin diagram in the IDE

//Sensor Up
unsigned int gTriggerAnalogOutChannelUp = 1; //channel to be connected to the module's TRIGGER pin - check the pin diagram in the IDE
unsigned int gEchoDigitalInPinUp = 2; //channel to be connected to the modules's ECHO pin (digital pin 2) - check the pin diagram in the IDE

int gTriggerCount = 0;
int gPrintfCount = 0;

float gFrequency = 0.0; // sensor Down frequency
float tFrequency = 0.0; // sensor Up frequency
float gPhase; // sensor Down phase
float tPhase; // sensor Up phase
float previousFrequencyDown = -1; // for sensor Down attenuation
float previousFrequencyUp = -1; // for sensor Up attenuation
float distanceDown = 0; // distance from sensor Down (down sensor)
float distanceUp = 0; // distance from sensor Up (up sensor)
float gInverseSampleRate;
float gTime = 0;
float frequenciesUp[5] = {233.0819, 277.1826, 311.1270, 369.9944, 415.3047}; // A#/Bflat C#/Dflat D#/Eflat F#/Gflat G#/Aflat  SENSOR UP
float frequenciesDown[10] = {261.625, 293.664, 329.627, 349.228, 391.995, 440.000, 493.993, 523.251}; // C D E F G A B C SENSOR DOWN
float attenuationConstantDown = 1.0;
float attenuationConstantUp = 1.0;
float attenuationRate = 6.5e-6;
int i = 0;


bool setup(BelaContext *context, void *userData)
{
    // Set the mode of digital pins
    pinMode(context, 0, gEchoDigitalInPinDown, INPUT); // receiving from ECHO PIN
    pinMode(context, 0, gEchoDigitalInPinUp, INPUT); // receiving from ECHO PIN
    
    pulseInDown.setup(context, gEchoDigitalInPinDown, HIGH); //detect HIGH pulses on this pin
    pulseInUp.setup(context, gEchoDigitalInPinUp, HIGH); //detect HIGH pulses on this pin
    
    
    scope.setup(2, 44100);
    if(context->analogInChannels != 8){
        fprintf(stderr, "This project has to be run with 8 analog channels\n");
        return false;
    }
    gInverseSampleRate = 1.0 / context->audioSampleRate;
	gPhase = 0.0;
	tPhase = 0.0;
	gFrequency = 0.0;
	tFrequency = 0.0;
	previousFrequencyDown = 0.0;
	previousFrequencyUp = 0.0;
	
    return true;
}

void render(BelaContext *context, void *userData)
{
    for(unsigned int n = 0; n<context->digitalFrames; n++){
        gTriggerCount++;
        if(gTriggerCount == gTriggerInterval){
            gTriggerCount = 0;
            if (yinyang == 0){
        		analogWriteOnce(context, n / 2, gTriggerAnalogOutChannelDown, HIGH); //write the status to the trig pin
        		yinyang = 1;
            }
            else {
        		analogWriteOnce(context, n / 2, gTriggerAnalogOutChannelUp, HIGH); //write the status to the trig pin
        		yinyang = 0;
            }
        } 
        else {
            analogWriteOnce(context, n / 2, gTriggerAnalogOutChannelDown, LOW); //write the status to the trig pin
            analogWriteOnce(context, n / 2, gTriggerAnalogOutChannelUp, LOW); //write the status to the trig pin
        }
        
        int pulseLengthDown = pulseInDown.hasPulsed(context, n); // will return the pulse duration(in samples) if a pulse just ended 
        int pulseLengthUp = pulseInUp.hasPulsed(context, n); // will return the pulse duration(in samples) if a pulse just ended 
        
        float durationDown = 1e6 * pulseLengthDown / context->digitalSampleRate; // pulse duration in microseconds
        float durationUp = 1e6 * pulseLengthUp / context->digitalSampleRate; // pulse duration in microseconds
        
        if(pulseLengthDown >= gMinPulseLength){
            static int countDown = 0;
            // rescaling according to the datasheet
            distanceDown = durationDown / gRescale;
            if(countDown > 50){ // we do not want to print the value every time we read it
            	rt_printf("Sensor: Down, pulseLength: %d, distance: %fcm\n", pulseLengthDown, distanceDown);
                countDown -= 50;
            }
         ++countDown;
         }
         
        if(pulseLengthUp >= gMinPulseLength){
            static int countUp = 0;
            // rescaling according to the datasheet
            distanceUp = durationUp / gRescale;
            if(countUp > 50){ // we do not want to print the value every time we read it
        		rt_printf("Sensor: Up, pulseLength: %d, distance: %fcm\n", pulseLengthUp, distanceUp);
                countUp -= 50;
            }
         ++countUp;
         }
    }
    
    if (distanceDown <= 24 && distanceDown > 0){
        gFrequency = frequenciesDown[int(distanceDown/3)];
    }
    else{
    	gFrequency = 0.0;
    }
    
    // if (distanceUp <= 24 && distanceUp > 0){
        // tFrequency = frequenciesUp[int((distanceUp/6)-1)];
    // }
    if (distanceUp <= 19){
    	// if (distanceUp >= 0 && distanceUp < 2) {
    	// 	tFrequency = frequenciesDown[0];
    	// }
    	if (distanceUp >= 2 && distanceUp <= 4){
    		tFrequency = frequenciesUp[0];
    	}
    	else if(distanceUp >= 5 && distanceUp <= 7){
    		tFrequency = frequenciesUp[1];
    	}
    	else if(distanceUp >= 11 && distanceUp <= 13){
    		tFrequency = frequenciesUp[2];
    	}
    	else if(distanceUp >= 14 && distanceUp <= 16){
    		tFrequency = frequenciesUp[3];
    	}
    	else if(distanceUp >= 17 && distanceUp <= 19){
    		tFrequency = frequenciesUp[4];
    	}
    }
    else{
    	tFrequency = 0.0;
    }
    
    for(unsigned int n = 0; n < context->audioFrames; n++) {
		float out;
		
		if (previousFrequencyDown == 0.0){
			previousFrequencyDown = gFrequency;
		}
		else if(previousFrequencyDown != gFrequency){
			previousFrequencyDown = gFrequency;
			attenuationConstantDown = 1.0;
		}
		
		if (previousFrequencyUp == 0.0){
			previousFrequencyUp = tFrequency;
		}
		else if(previousFrequencyUp != tFrequency){
			previousFrequencyUp = tFrequency;
			attenuationConstantUp = 1.0;
		}

		gPhase += 2.0f * (float)M_PI * gFrequency * gInverseSampleRate;
		if(gPhase > M_PI)
			{
			gPhase -= 2.0f * (float)M_PI;
			// rt_printf("UP Phase:%f\n", gPhase);
			}
		tPhase += 2.0f * (float)M_PI * tFrequency * gInverseSampleRate;
		if(tPhase > M_PI)
			{
			tPhase -= 2.0f * (float)M_PI;
			// rt_printf("DOWN Phase:%f\n", tPhase);
			}
		out = (attenuationConstantDown * sinf(gPhase)) + (attenuationConstantUp * sinf(tPhase));
		
		audioWrite(context, n, 1, out);
		attenuationConstantDown = max(0.0, attenuationConstantDown - attenuationRate);
		attenuationConstantUp = max(0.0, attenuationConstantUp - attenuationRate);
	}
	
	gTime++;
        // Logging to the scope the pulse inputs (gEchoDigitalInPin) and the distance
        // scope.log(digitalRead(context, n, gEchoDigitalInPin), distance/100);
}


void cleanup(BelaContext *context, void *userData){
	
}
