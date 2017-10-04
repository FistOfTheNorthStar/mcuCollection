#include <stdint.h>
#include <ffft.h>
#include <math.h>

/*
PICCOLO is a tiny Arduino-based audio visualizer.

Hardware requirements:
 - Most Arduino or Arduino-compatible boards (ATmega 328P or better).

Software requirements:
 - elm-chan's ffft library for Arduino

Connections:
 - 3.3V to mic amp+ and Arduino AREF pin <-- important!
 - GND to mic amp-
 - Analog pin 0 to mic amp output
 
ffft library is provided under its own terms -- see ffft.S for specifics.
*/

// IMPORTANT: FFT_N should be #defined as 128 in ffft.h.

// Microphone connects to Analog Pin 0.  Corresponding ADC channel number
// varies among boards...it's ADC0 on Uno and Mega, ADC7 on Leonardo.
// Other boards may require different settings; refer to datasheet.
//#ifdef __AVR_ATmega32U4__
// #define ADC_CHANNEL 7
//#else
 #define ADC_CHANNEL 0
//#endif

int16_t       threshold = 0;      //compare to this when taking a reading
int16_t       capture[FFT_N];    // Audio capture buffer
complex_t     bfly_buff[FFT_N];  // FFT "butterfly" buffer
uint16_t      spectrum[FFT_N/2]; // Spectrum output buffer
volatile byte samplePos = 0;     // Buffer position counter
volatile unsigned long volAddloop = 0;    //gather sound volume
unsigned long toSerialVol = 0;
int           iteratorVol = 0;
unsigned long interval = 5000; //interval on when to calculate BPM
volatile unsigned long previousMillis=0; // millis() returns an unsigned long.
int colBPM[75]; //gather a second worth of BPM
volatile unsigned long currentMillis = 0;
byte colBPMiterator = 0; // iterate through the bpm calc.
int BPM5s= 0; //we will calculate enough beats per 5s then multiply
unsigned long BPMbaseline = 0;
boolean beatStarted = false;
byte micIn = A0;

void setup() {
  
  

  memset(colBPM , 0, sizeof(colBPM));

  //calculate what is silent
  int16_t totalNoise = 0;
  int iteratorNoise = 0;
  unsigned long timeToTest = millis() + 6600;
  while (millis() < timeToTest){
      //totalNoise += analogRead(ADC_CHANNEL);
      totalNoise += 12;
      iteratorNoise++;
      delay(20);
  }

  delay(1000);
  
  //silence downscale by 10
  threshold = totalNoise / iteratorNoise;

  Serial.begin(57600);
}

void loop() {
 
  unsigned long start = micros();

  fft_input(capture, bfly_buff);   // Samples -> complex #s
  samplePos = 0;                   // Reset sample counter
 
  fft_execute(bfly_buff);          // Process complex data
  fft_output(bfly_buff, spectrum); // Complex -> spectrum

  //bpm roller for average 75Hz resolution so these would add 75, 150, 225 enough for bass
  colBPM[colBPMiterator]  = spectrum[1] + spectrum[2] + spectrum[3];
  colBPMiterator++;

  //calculate the BPM average to compare to, not entirely rolling, but probably will do for now
  if (colBPMiterator == 75){
    BPMbaseline = 0;
    for(int i=0;i<75;i++){
      BPMbaseline += colBPM[i];
    }
    
  }

  //now let's check the BPM, if the peak goes above the average by 1.5 then we count it as a beat
  if(colBPM[colBPMiterator] > 1.5 * BPMbaseline){
        beatStarted = true;
  }else{
        //now count as one beat
        if(beatStarted){
          BPM5s++;
        }
        beatStarted = false;
  }

  iteratorVol++;
 
  //this is to add the sound volume then send it at some point
  if(iteratorVol < 50){
    iteratorVol = 0; 
    toSerialVol = volAddloop / 50;
    volAddloop = 0;   
  }

Serial.println(volAddloop);

    currentMillis = millis(); // grab current time
    // check if "interval" time has passed
    //Serial.println(currentMillis - previousMillis);
    //read temperature every e.g. 20s and humidity every 20s
    if ((unsigned long)(currentMillis - previousMillis) >= interval) {
               
        Serial.println("BPM "+(String)(BPM5s*60000/interval) +" VOL " + (String)toSerialVol);

        //save the "current" time
        previousMillis = millis();
        BPM5s= 0; //reset BPM calculator
    }
   
   unsigned long end = micros();
   Serial.println(end-start);
  
}

void readVOL() { // Audio-sampling interrupt
  for (byte i =0;i<128;i++){
    int16_t sample = analogRead(micIn); // 0-1023

    capture[samplePos] = sample - threshold;

  samplePos++;
  if(samplePos == 128);
  
  volAddloop += sq(sample)/10;
  Serial.println(sample);
  }
}

