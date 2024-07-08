#include <Arduino.h>
#include "ad9833.h"

AD9833_IC generator(1, 5, 3); // creates new object with pins 1,5,3 as SCK, FSYNC, DATA
                                // and initialises IC with stopped state 
                                // (reset and sleep states are ON);

void setup(){
  generator.setFrequency(4000, 0); // sets 4kHz frequency in the register 0;
  generator.start(0,0,0); // starts generation with sine wave with 
                          // selected frequency and phase registers (0 and 0).
}

void loop(){
	generator.stop(); // stops generation;
	delay(5000); 
	generator.start(); // starts generation again;
	delay(5000); 
}
