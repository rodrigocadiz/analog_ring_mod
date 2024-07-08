/* 
Ring modulator firmware made by Michel Rozas <mvrozas@uc.cl> for ESP32 WROOM 32D microcontroller

Open source libraries used in this project:

- Library for daumemo.com for AD9833 by Analog Devices
- IIR Filter Library - Copyright (C) 2016  Martin Vincent Bloedorn
- ESP32Encoder - https://registry.platformio.org/libraries/madhephaestus/ESP32Encoder
- SmartLEDS - https://registry.platformio.org/libraries/roboticsbrno/SmartLeds
- SSD1306 OLED screen - https://registry.platformio.org/libraries/adafruit/Adafruit%20SSD1306
- ADS1115 ADC - https://registry.platformio.org/libraries/adafruit/Adafruit%20ADS1X15
*/

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <filters.h>
#include <Adafruit_ADS1X15.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SmartLeds.h>
#include <ESP32Encoder.h>  
#include "DacESP32.h"
#include "ad9833.h"
#include "DFRobotDFPlayerMini.h"


#define ENCODER1_A 23        // CLK ENCODER 1
#define ENCODER1_B 27        // DT ENCODER  1

#define ENCODER2_A 35        // CLK ENCODER 2
#define ENCODER2_B 34        // DT ENCODER  2

#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels

#define I2C_SDA1 21          // I2C Screen and ADC pins
#define I2C_SCL1 22          // I2C Screen and ADC pins

#define MUX_S0  5            // MUX pin s0
#define MUX_S1 13            // MUX pin s1
#define MUX_S2 19            // MUX pin s2
#define MUX_S3 18            // MUX pin s3

#define DFPLAYER_TX 16       // Wav player Tx pin
#define DFPLAYER_RX 17       // Wav player Rx pin

//Instance the DFrobot player mini 
DFRobotDFPlayerMini DFPlayer;

const int LED_COUNT = 16;     // Number of LEDs
const int DATA_PIN = 2;       // LED strip pin
const int CHANNEL = 0;        // LED strip channel

//Tasks for RTOS
TaskHandle_t Task1;
TaskHandle_t Task2;

//TwoWire begin
TwoWire I2C1 = TwoWire(0);
TwoWire I2C2 = TwoWire(1);

//Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C1, 4);

//ADS1115 ADC init
Adafruit_ADS1115 ads;        

//ESP32 DAC 
DacESP32 dac1(DAC_CHANNEL_1);

  
// SmartLed -> RMT driver (WS2812/WS2812B/SK6812/WS2813)
SmartLed leds(LED_WS2812B, LED_COUNT, DATA_PIN, CHANNEL, DoubleBuffer);

//Instance for the encoders
ESP32Encoder encoder1;
ESP32Encoder encoder2;


//Global Variables
float out = 0;           // Frequency selected after mapping
int disc_in = 0;         // potentiometer quantization to 8 bits
uint8_t led_state = 0;   // Selected LED set
uint8_t number = 0;      // Number reated to Mantra configurations
uint8_t mode = 0 ;       // Mode of mapping
uint8_t mux_output = 0;  // Mux output value 
uint8_t screen_flag = 0; // Flag to detect if a mode change occurred
uint8_t input_adc = 0;   // 0: Potentiometer   1: Slider
long newPosition = 0;    // Flag for encoder changes 
float input_freq = 0;    // Freq (is routed to Modulator)
uint8_t color_r = 0;     // Set the led colors
uint8_t color_g = 0;     // Set the led colors
uint8_t color_b = 50;    // Set the led colors
String note;             // string to store the value of each note in American key

// Frecuencias en Hz (multiplicadas por 2)
float freq1  = 184.0;    // Gb2    12 
float freq2  = 220.0;    // A2     1
float freq3  = 233.0;    // Bb2    11
float freq4  = 246.94;   // B2     2
float freq5  = 261.6;    // C3     10
float freq6  = 277.18;   // Db3    9
float freq7  = 293.66;   // D3     6
float freq8  = 311.12;   // Eb3    8
float freq9  = 329.62;   // E3     4
float freq10 = 349.22;   // F3     5
float freq11 = 391.96;   // G3     7
float freq12 = 415.3;    // G#3    3
float freq13 = 369.0;    // F#3    13 ???     
float freq14 = 440.0;    // A3     14 ???
float freq15 = 880.0;    // A4     15 ???
float freq16 = 3520.0;   // A6     16 ???

// Frecuencias en Hz (multiplicadas por 2)
float m3freq1  = 116.54;  // Bb1   3  
float m3freq2  = 123.48;  // B1    7
float m3freq3  = 138.6;   // C#2   5
float m3freq4  = 146.84;  // D2    4
float m3freq5  = 155.56;  // Eb2   8
float m3freq6  = 164.82;  // E2    6
float m3freq7  = 174.62;  // F2    9
float m3freq8  = 185.0;   // Gb2   10
float m3freq9  = 196.0;   // G2    2
float m3freq10 = 207.66;  // Ab2   11
float m3freq11 = 220.0;   // A2    1
float m3freq12 = 261.62;  // C3    12
float m3freq13 = 329.0;   // E3    13 ???
float m3freq14 = 415.0;   // G#3   14 ???
float m3freq15 = 880.0;   // A4    15 ???
float m3freq16 = 1760.0;  // A5    16 ???

// Select values between 0-255 to define the pot trheshols
uint8_t m1th1  = 25 ;
uint8_t m1th2  = 50 ;
uint8_t m1th3  = 75 ;
uint8_t m1th4  = 90 ;
uint8_t m1th5  = 105 ;
uint8_t m1th6  = 120 ;
uint8_t m1th7  = 145 ;
uint8_t m1th8  = 160 ;
uint8_t m1th9  = 175 ;
uint8_t m1th10 = 190 ;
uint8_t m1th11 = 210 ;
uint8_t m1th12 = 230 ;

// Select values between 0-255 to define the pot thresholds
uint8_t m2th1  = 9 ;    // 0-9    : Constant in Gb2
uint8_t m2th2  = 18 ;   // 9-18   : Linear mapping between Gb2 / A2
uint8_t m2th3  = 27 ;   // 18-27  : Constant in A2
uint8_t m2th4  = 36 ;   // 27-36  : Linear mapping between A2 / Bb2
uint8_t m2th5  = 45 ;   // 36-45  : Constant in Bb2
uint8_t m2th6  = 54 ;   // 45-54  : Linear mapping between Bb2 / B2
uint8_t m2th7  = 63 ;   // 54-63  : Constant in B2
uint8_t m2th8  = 72 ;   // 63-72  : Linear mapping between B2 / C3
uint8_t m2th9  = 81 ;   // 72-81  : Constant in C3
uint8_t m2th10 = 90 ;   // 81-90  : Linear mapping between C3 / Db3
uint8_t m2th11 = 99 ;   // 90-99  : Constant in Db3
uint8_t m2th12 = 108 ;  // 99-108 : Linear mapping between Db3 / D3
uint8_t m2th13 = 117 ;  // 108-117: Constant in D3
uint8_t m2th14 = 126 ;  // 117-126: Linear mapping between D3 / Eb3
uint8_t m2th15 = 135 ;  // 126-135: Constant in Eb3
uint8_t m2th16 = 144 ;  // 135-144: Linear mapping between Eb3 / E3
uint8_t m2th17 = 153 ;  // 144-153: Constant in E3
uint8_t m2th18 = 162 ;  // 153-162: Linear mapping between E3 / F3
uint8_t m2th19 = 171 ;  // 162-171: Constant in F3
uint8_t m2th20 = 180 ;  // 171-180: Linear mapping between F3 / G3
uint8_t m2th21 = 189 ;  // 180-189: Constant in G3
uint8_t m2th22 = 198 ;  // 189-198: Linear mapping between G3 / G#3
uint8_t m2th23 = 207 ;  // 198-207: Constant in G#3
uint8_t m2th24 = 216 ;  // 207-216: Constant in F#3
uint8_t m2th25 = 225 ;  // 216-225: Constant in A3
uint8_t m2th26 = 234 ;  // 225-234: Constant in A4
uint8_t m2th27 = 255 ;  // 234-255: Constant in A6

                     

// Select values between 0-255 to define the pot thresholds
uint8_t m3th1  = 9 ;    // 0-9     : Constant in Bb1
uint8_t m3th2  = 18 ;   // 9-18    : Linear mapping between Bb1 / B1
uint8_t m3th3  = 27 ;   // 18-27   : Constant in B1
uint8_t m3th4  = 36 ;   // 27-36   : Linear mapping between B1 / C#2
uint8_t m3th5  = 45 ;   // 36-45   : Constant in C#2
uint8_t m3th6  = 54 ;   // 45-54   : Linear mapping between C#2 / D2
uint8_t m3th7  = 63 ;   // 54-63   : Constant in D2
uint8_t m3th8  = 72 ;   // 63-72   : Linear mapping between D2 / Eb2
uint8_t m3th9  = 81 ;   // 72-81   : Constant in Eb2
uint8_t m3th10 = 90 ;   // 81-90   : Linear mapping between Eb2 / E2
uint8_t m3th11 = 99 ;   // 90-99   : Constant in E2
uint8_t m3th12 = 108 ;  // 99-108  : Linear mapping between E2 / F2
uint8_t m3th13 = 117 ;  // 108-117 : Constant in F2
uint8_t m3th14 = 126 ;  // 117-126 : Linear mapping between F2 / Gb2
uint8_t m3th15 = 135 ;  // 126-135 : Constant in Gb2
uint8_t m3th16 = 144 ;  // 135-144 : Linear mapping between Gb2 / G2
uint8_t m3th17 = 153 ;  // 144-153 : Constant in G2
uint8_t m3th18 = 162 ;  // 153-162 : Linear mapping between G2 / Ab2
uint8_t m3th19 = 171 ;  // 162-171 : Constant in Ab2
uint8_t m3th20 = 180 ;  // 171-180 : Linear mapping between Ab2 / A2
uint8_t m3th21 = 189 ;  // 180-189 : Constant in A2
uint8_t m3th22 = 198 ;  // 189-198 : Linear mapping between A2 / C3
uint8_t m3th23 = 207 ;  // 198-207 : Constant in C3
uint8_t m3th24 = 216 ;  // 207-216 : Constant in E3
uint8_t m3th25 = 225 ;  // 216-225 : Constant in G#3
uint8_t m3th26 = 234 ;  // 225-234 : Constant in A4
uint8_t m3th27 = 255 ;  // 234-255 : Constant in A5


void off_leds(){
  for(int i=0;i<16;i++){
    leds[i] = Rgb { 0, 0, 0 };
  }
}

void show_leds(){
  if(mode == 0)      { color_b=100;color_g=0;color_r=0; }
  else if (mode == 1){ color_b=0;color_g=100;color_r=0; }
  else if (mode == 2){ color_b=0;color_g=0;color_r=100; }
  else if (mode == 3){ color_b=100;color_g=100;color_r=0; }
  else if (mode == 4){ color_b=100;color_g=0;color_r=100; }
  else if (mode == 5){ color_b=120;color_g=70;color_r=0; }
  else               { color_b=100;color_g=100;color_r=100;}
  off_leds();
  leds[15 - led_state] = Rgb { color_r, color_g, color_b};
  if(led_state == 0){
    off_leds();
  }
  leds.show();
}

float map_pot(int in){
  
  int threshold = 17000;
  disc_in = map(in,0,threshold,0,255);

  //Modo 0: Saltos discretos entre frecuencias
  if ( mode == 0){
    if(disc_in>=0    && disc_in <m1th1) {out=freq1; led_state = 2;}
    else if(disc_in>=m1th1  && disc_in <m1th2) {out=freq2; led_state = 3;}
    else if(disc_in>=m1th2  && disc_in <m1th3) {out=freq3; led_state = 4;}
    else if(disc_in>=m1th3  && disc_in <m1th4) {out=freq4; led_state = 5;}
    else if(disc_in>=m1th4  && disc_in <m1th5) {out=freq5; led_state = 6;}
    else if(disc_in>=m1th5  && disc_in <m1th6) {out=freq6; led_state = 7;}
    else if(disc_in>=m1th6  && disc_in <m1th7) {out=freq7; led_state = 8;}
    else if(disc_in>=m1th7  && disc_in <m1th8) {out=freq8; led_state = 9;}
    else if(disc_in>=m1th8  && disc_in <m1th9) {out=freq9; led_state = 10;}
    else if(disc_in>=m1th9  && disc_in <m1th10){out=freq10;led_state = 11;}
    else if(disc_in>=m1th10 && disc_in <m1th11){out=freq11;led_state = 12;}
    else if(disc_in>=m1th11 && disc_in <m1th12){out=freq12;led_state = 13;}
    else if(disc_in>=m1th12)                 {out=freq13;led_state = 14;}
    else { out = 0;}
  }

  //Mode 1: Piano 1 (Normal mode) Gradual jumps between frequencies (Mapping with linear interpolation)
  else if (mode == 1){
    if(disc_in>=0    && disc_in <m2th1) {out=freq1; led_state = 2; number = 12;note = "Gb2";}
    else if(disc_in>=m2th1  && disc_in <m2th2) {out=map(disc_in,m2th1,m2th2,freq1,freq2); led_state = 0;number = 0;}
    else if(disc_in>=m2th2  && disc_in <m2th3) {out=freq2; led_state = 2; number = 1;note = "A2";}
    else if(disc_in>=m2th3  && disc_in <m2th4) {out=map(disc_in,m2th3,m2th4,freq2,freq3); led_state = 0;number = 0;}
    else if(disc_in>=m2th4  && disc_in <m2th5) {out=freq3; led_state = 3; number = 11;note = "Bb2";}
    else if(disc_in>=m2th5  && disc_in <m2th6) {out=map(disc_in,m2th5,m2th6,freq3,freq4); led_state = 0;number = 0;}
    else if(disc_in>=m2th6  && disc_in <m2th7) {out=freq4; led_state = 4; number = 2;note = "B2";}
    else if(disc_in>=m2th7  && disc_in <m2th8) {out=map(disc_in,m2th7,m2th8,freq4,freq5); led_state = 0;number = 0;}
    else if(disc_in>=m2th8  && disc_in <m2th9) {out=freq5; led_state = 5; number = 10;note = "C3";}
    else if(disc_in>=m2th9  && disc_in <m2th10){out=map(disc_in,m2th9,m2th10,freq5,freq6); led_state = 0;number = 0;}
    else if(disc_in>=m2th10 && disc_in <m2th11){out=freq6; led_state = 6; number = 9;note = "Db3";}
    else if(disc_in>=m2th11 && disc_in <m2th12){out=map(disc_in,m2th11,m2th12,freq6,freq7); led_state = 0;number = 0;}
    else if(disc_in>=m2th12 && disc_in <m2th13){out=freq7; led_state = 7; number = 6;note = "D3";}
    else if(disc_in>=m2th13 && disc_in <m2th14){out=map(disc_in,m2th13,m2th14,freq7,freq8); led_state = 0;number = 0;}
    else if(disc_in>=m2th14 && disc_in <m2th15){out=freq8; led_state = 8; number = 8;note = "Eb3";}
    else if(disc_in>=m2th15 && disc_in <m2th16){out=map(disc_in,m2th15,m2th16,freq8,freq9); led_state = 0;number = 0;}
    else if(disc_in>=m2th16 && disc_in <m2th17){out=freq9; led_state = 9; number = 4;note = "E3";}
    else if(disc_in>=m2th17 && disc_in <m2th18){out=map(disc_in,m2th17,m2th18,freq9,freq10); led_state = 0;number = 0;}
    else if(disc_in>=m2th18 && disc_in <m2th19){out=freq10; led_state = 10; number = 5;note = "F3";}
    else if(disc_in>=m2th19 && disc_in <m2th20){out=map(disc_in,m2th19,m2th20,freq10,freq11); led_state = 0;number = 0;}
    else if(disc_in>=m2th20 && disc_in <m2th21){out=freq11; led_state = 11; number = 7;note = "G3";}
    else if(disc_in>=m2th21 && disc_in <m2th22){out=map(disc_in,m2th21,m2th22,freq11,freq12); led_state = 0;number = 0;}
    else if(disc_in>=m2th22 && disc_in <m2th23){out=freq12; led_state = 12;number = 3;note = "G#3";}
    else if(disc_in>=m2th23 && disc_in <m2th24){out=freq13; led_state = 0;number = 13;note = "F#3";}
    else if(disc_in>=m2th24 && disc_in <m2th25){out=freq14; led_state = 0;number = 14;note = "A3";}
    else if(disc_in>=m2th25 && disc_in <m2th26){out=freq15; led_state = 0;number = 15;note = "A4";}
    else if(disc_in>=m2th26 && disc_in <m2th27){out=freq16; led_state = 0;number = 16;note = "A6";}

    else { out = 0;}
  }
  
  //Mode 2: Piano 2 (Normal mode) Gradual jumps between frequencies (Mapping with linear interpolation)
  else if (mode == 2){
    if(disc_in>=0    && disc_in <m3th1) {out=m3freq1; led_state = 2; number = 3;note = "Bb1";}
    else if(disc_in>=m3th1  && disc_in <m3th2) {out=map(disc_in,m3th1,m3th2,m3freq1,m3freq2); led_state = 0;number = 0;}
    else if(disc_in>=m3th2  && disc_in <m3th3) {out=m3freq2; led_state = 2; number = 7;note = "B1";}
    else if(disc_in>=m3th3  && disc_in <m3th4) {out=map(disc_in,m3th3,m3th4,m3freq2,m3freq3); led_state = 0;number = 0;}
    else if(disc_in>=m3th4  && disc_in <m3th5) {out=m3freq3; led_state = 3; number = 5;note = "C#2";}
    else if(disc_in>=m3th5  && disc_in <m3th6) {out=map(disc_in,m3th5,m3th6,m3freq3,m3freq4); led_state = 0;number = 0;}
    else if(disc_in>=m3th6  && disc_in <m3th7) {out=m3freq4; led_state = 4; number = 4;note = "D2";}
    else if(disc_in>=m3th7  && disc_in <m3th8) {out=map(disc_in,m3th7,m3th8,m3freq4,m3freq5); led_state = 0;number = 0;}
    else if(disc_in>=m3th8  && disc_in <m3th9) {out=m3freq5; led_state = 5; number = 8;note = "Eb2";}
    else if(disc_in>=m3th9  && disc_in <m3th10){out=map(disc_in,m3th9,m3th10,m3freq5,m3freq6); led_state = 0;number = 0;}
    else if(disc_in>=m3th10 && disc_in <m3th11){out=m3freq6; led_state = 6; number = 6;note = "E2";}
    else if(disc_in>=m3th11 && disc_in <m3th12){out=map(disc_in,m3th11,m3th12,m3freq6,m3freq7); led_state = 0;number = 0;}
    else if(disc_in>=m3th12 && disc_in <m3th13){out=m3freq7; led_state = 7; number = 9;note = "F2";}
    else if(disc_in>=m3th13 && disc_in <m3th14){out=map(disc_in,m3th13,m3th14,m3freq7,m3freq8); led_state = 0;number = 0;}
    else if(disc_in>=m3th14 && disc_in <m3th15){out=m3freq8; led_state = 8; number = 10;note = "Gb2";}
    else if(disc_in>=m3th15 && disc_in <m3th16){out=map(disc_in,m3th15,m3th16,m3freq8,m3freq9); led_state = 0;number = 0;}
    else if(disc_in>=m3th16 && disc_in <m3th17){out=m3freq9; led_state = 9; number = 2;note = "G2";}
    else if(disc_in>=m3th17 && disc_in <m3th18){out=map(disc_in,m3th17,m3th18,m3freq9,m3freq10); led_state = 0;number = 0;}
    else if(disc_in>=m3th18 && disc_in <m3th19){out=m3freq10; led_state = 10;number = 11;note = "Ab2";}
    else if(disc_in>=m3th19 && disc_in <m3th20){out=map(disc_in,m3th19,m3th20,m3freq10,m3freq11); led_state = 0;number = 0;}
    else if(disc_in>=m3th20 && disc_in <m3th21){out=m3freq11; led_state = 11; number = 1;note = "A2";}
    else if(disc_in>=m3th21 && disc_in <m3th22){out=map(disc_in,m3th21,m3th22,m3freq11,m3freq12); led_state = 0;number = 0;}
    else if(disc_in>=m3th22 && disc_in <m3th23){out=m3freq12; led_state = 12; number = 12;note = "C3";}
    else if(disc_in>=m3th23 && disc_in <m3th24){out=m3freq13; led_state = 0; number = 13;note = "E3";}
    else if(disc_in>=m3th24 && disc_in <m3th25){out=m3freq14; led_state = 0; number = 14;note = "G#3";}
    else if(disc_in>=m3th25 && disc_in <m3th26){out=m3freq15; led_state = 0; number = 15;note = "A4";}
    else if(disc_in>=m3th26 && disc_in <m3th27){out=m3freq16; led_state = 0; number = 16;note = "A5";}
    else { out = 0;}
  }

  //Modo 3: recorrido continuo entre 1 Hz y 5 kHz 
  else if (mode == 3){
    out = map(in,0,threshold,0,5000);
    led_state = map(in,0,threshold,2,15);
  }

  //Modo 4: recorrido continuo entre 1 Hz y 1 kHz 
  else if (mode == 4){
    out = map(in,0,threshold,0,1000);
    led_state = map(in,0,threshold,2,15);
  }

  //Modo 5: recorrido continuo entre 1 Hz y 500 Hz 
  else if (mode == 5){
    out = map(in,0,threshold,0,500);
    led_state = map(in,0,threshold,2,15);
  }

  //En cualquier otro caso, se apaga el oscilador local 
  else{
    out = 0;
    led_state = 0;
  }
  return out;
}

void update_display(){
  
  if ((mode == 1 || mode == 2)){ 
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(5,1);
    if(mode == 1){
      display.print("MODE:Mantra-Piano I");
    }
    else{
      display.print("MODE:Mantra-Piano II");
    }
    display.setTextSize(2);
    display.setCursor(15, 20);
    display.print("NOTE");
    display.setCursor(85, 20);
    display.print("Nro");
    if(number == 0){
      display.setCursor(15, 45);
    }
    else{
      display.setCursor(22, 45);
      display.print(note);
      display.setCursor(95, 45);
      display.print(number);
    }
    display.display(); 
    screen_flag = 1;
  }

  else if (mode == 3 || mode == 4 || mode == 5){
    if(screen_flag == 1){
      display.clearDisplay();
      display.setCursor(40, 10);
      display.print("MODE");
      display.setCursor(5, 40);
      display.print("CONTINUOUS");
      display.display();
      screen_flag = 0; //Show only once
    }
  }

  else{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(5,1);
    display.print("MODE: Test");
    display.setTextSize(2);
    display.setCursor(5, 20);
    display.print("Frecuencia");
    display.setCursor(5, 45);
    display.print(input_freq);
    display.setCursor(100, 40);
    display.print("Hz");
    display.display(); 
    screen_flag = 1;
  }
}

void change_mux(int val) {
    switch(val) {
        case 0:  // AM
            digitalWrite(MUX_S0, LOW);
            digitalWrite(MUX_S1, LOW);
            digitalWrite(MUX_S2, LOW);
            digitalWrite(MUX_S3, LOW);
            break;
        case 1: // FM
            digitalWrite(MUX_S0, HIGH);
            digitalWrite(MUX_S1, LOW);
            digitalWrite(MUX_S2, LOW);
            digitalWrite(MUX_S3, LOW);
            break;
        case 2:  // BYPASS
            digitalWrite(MUX_S0, LOW);
            digitalWrite(MUX_S1, HIGH);
            digitalWrite(MUX_S2, LOW);
            digitalWrite(MUX_S3, LOW);
            break;
        case 3:  // LOCAL SG
            digitalWrite(MUX_S0, HIGH);
            digitalWrite(MUX_S1, HIGH);
            digitalWrite(MUX_S2, LOW);
            digitalWrite(MUX_S3, LOW);
            break;
        default:
            // Opcional: manejar valores fuera del rango esperado
            break;
    }
}

void frequency_generator(void *param){

  encoder1.attachSingleEdge (ENCODER1_A, ENCODER1_B);
  encoder1.setCount ( 0 );
  
  encoder2.attachSingleEdge (ENCODER2_A, ENCODER2_B);
  encoder2.setCount ( 0 );


  //Define main variables 
  float filtered_volts = 0;
  float filtered_adc   = 0;
  float filtered_out = 0;

  //Define ADC variables 
  int16_t adc0, adc1, adc2, adc3 = 0;
  float volts0, volts1, volts2, volts3 = 0;
 
  //Define IIR filter parameters
  const float cutoff_freq   = 5.0;  //Cutoff frequency in Hz
  const float sampling_time = 0.005; //Sampling time in seconds.
  IIR::ORDER  order  = IIR::ORDER::OD2; // Order (OD1 to OD4)
  
  // Low-pass filter
  Filter f(cutoff_freq, sampling_time, order);

  //AD9833 init 
  //AD9833_IC generator(32, 33, 25); // creates new object with pins 1,5,3 as SCK, FSYNC, DATA
                                     // and initialises IC with stopped state 
                                     // (reset and sleep states are ON);
  AD9833_IC generator(33, 26, 32);

  generator.setFrequency(4000, 0); // sets 4kHz frequency in the register 0;
  generator.setWaveForm(1);
  generator.start(0,0,0);          // starts generation with sine wave with 
                                   // selected frequency and phase registers (0 and 0).

  while(true){
    
    //Read ADC values 
    adc0 = ads.readADC_SingleEnded(0);
    adc1 = ads.readADC_SingleEnded(1);


    //Compute data from bit to voltage
    volts0 = ads.computeVolts(adc0);
    volts1 = ads.computeVolts(adc1);

  
    //Serial.println("-----------------------------------------------------------");
    //Serial.print("AIN0: "); Serial.print(adc0); Serial.print("  "); Serial.print(volts0); Serial.println("V");
    //Serial.print("AIN1: "); Serial.print(adc1); Serial.print("  "); Serial.print(volts1); Serial.println("V");
    //Serial.print("AIN2: "); Serial.print(adc2); Serial.print("  "); Serial.print(volts2); Serial.println("V");
    //Serial.print("AIN3: "); Serial.print(adc3); Serial.print("  "); Serial.print(volts3); Serial.println("V");
    
    //Main code
    
    if (input_adc == 0){
      filtered_adc = f.filterIn(adc0);
    }
    else{
      filtered_adc = f.filterIn(adc1);
    }
    
    if(filtered_adc < 0){
      filtered_adc = 0;
    }
    filtered_out   = map_pot(filtered_adc); //Modo discreto
    input_freq = filtered_out;

    show_leds();
    //input_freq = map(filtered_adc0,0,9150,0,1000);
    //input_freq = (filtered_adc0/9150.0)*6000.0;
    
    uint8_t pmod = encoder1.getCount();
    
    if(pmod ==  255) { mode = 1;}
    else if(pmod ==  254) { mode = 2;}
    else if(pmod ==  253) { mode = 3;}
    else if(pmod ==  252) { mode = 4;}
    else if(pmod ==  251) { mode = 5;}
    else{mode  = 0;}

    //Serial.println(mode);

    //mux_output = abs(encoder2.getCount());
    //Serial.println(newPosition);
    
    //Serial.print("Modo: ");
    //Serial.print(mode);
    //Serial.print(" , mux: ");
    //Serial.print(mux_output);
    //Serial.print(" Valor pot: ");
    //Serial.print(filtered_adc0, 4);
    //Serial.print(", Frecuencia generador: ");
    //Serial.println(input_freq);
    
    generator.setFrequency(input_freq,0);
    //dac1.outputCW(input_freq/4);

    generator.start();
    
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void UI(void *param){
  
  while(true){
    update_display();
    
    change_mux(mux_output);
    
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void setup(){
  
  pinMode(MUX_S0,OUTPUT);
  pinMode(MUX_S1,OUTPUT);
  pinMode(MUX_S2,OUTPUT);
  pinMode(MUX_S3,OUTPUT);


  Serial.begin(115200);
  
  Serial2.begin(9600,SERIAL_8N1,16,17);

  //if (!DFPlayer.begin(Serial2, /*isACK = */true, /*doReset = */true)) {  
  //  Serial.println(F("Unable to begin:"));
  //  Serial.println(F("1.Please recheck the connection!"));
  //  Serial.println(F("2.Please insert the SD card!"));
  //}

  //Serial.println(F("DFPlayer Mini online."));

  //DFPlayer.volume(1);

  //DFPlayer.play(1);
  //delay(5000);
  //DFPlayer.pause();
  
  //dac1.setCwScale(DAC_CW_SCALE_8);

  
  I2C1.begin(I2C_SDA1, I2C_SCL1, 100000UL);

  if (!ads.begin(0x48,&I2C1)){
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)"); // Print ADC range 

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  
  xTaskCreatePinnedToCore(frequency_generator,"Task1",4096*2,NULL,1,&Task1,1);
  xTaskCreatePinnedToCore(UI                 ,"Task2",4096,NULL,1,&Task2,0);

}

void loop(){
  
}
